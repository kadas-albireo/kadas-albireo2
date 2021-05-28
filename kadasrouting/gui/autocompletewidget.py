import json
import logging
import re
from PyQt5.QtCore import Qt, pyqtSignal, QUrl, QUrlQuery
from PyQt5.QtCore import QObject, QTimer, QEvent, QPoint, QMetaObject
from PyQt5.QtWidgets import QTreeWidget, QLineEdit, QFrame, QTreeWidgetItem
from PyQt5.QtNetwork import QNetworkAccessManager, QNetworkRequest, QNetworkReply
from PyQt5.QtGui import QPalette

from qgis.core import QgsSettings

from kadasrouting.utilities import strip_tags

LOG = logging.getLogger(__name__)


class SuggestCompletion(QObject):

    finished = pyqtSignal(dict)
    error = pyqtSignal(str)

    def __init__(self, parent):
        QObject.__init__(self, parent)
        self._parent = parent
        # editor (a QLineEdit)
        self._editor = parent
        # pop up
        self._popup = QTreeWidget()
        self._popup.setWindowFlags(Qt.Popup)
        self._popup.setFocusProxy(self._parent)
        self._popup.setMouseTracking(True)
        self._popup.setColumnCount(1)
        self._popup.setUniformRowHeights(True)
        self._popup.setRootIsDecorated(False)
        self._popup.setEditTriggers(QTreeWidget.NoEditTriggers)
        self._popup.setSelectionBehavior(QTreeWidget.SelectRows)
        self._popup.setFrameStyle(QFrame.Box | QFrame.Plain)
        self._popup.setHorizontalScrollBarPolicy(Qt.ScrollBarAlwaysOff)
        self._popup.header().hide()
        # timer
        self._timer = None
        self._timer = QTimer(self)
        self._timer.setSingleShot(True)
        self._timer.setInterval(500)
        # network manager
        self._network_manager = QNetworkAccessManager(self)

        # signal and slot
        self._popup.installEventFilter(self)
        self._popup.itemClicked.connect(self.done_completion)
        self._timer.timeout.connect(self.auto_suggest)
        self._editor.textEdited.connect(self._timer.start)
        self._network_manager.finished.connect(self.handle_network_data)

    def eventFilter(self, object, event):
        print('nope')
        if object != self._popup or object != self._editor:
            return False

        if event.type() == QEvent.MouseButtonPress:
            self._popup.hide()
            self._editor.setFocus()
            return True

        if event.type() == QEvent.KeyPress:
            consumed = False
            key = event.key()
            if key in [Qt.Key_Enter, Qt.Key_Return]:
                self.done_completion()
                consumed = True
            elif key == Qt.Key_Escape:
                self._editor.setFocus()
                self._popup.hide()
                consumed = True
            elif key in [
                Qt.Key_Up,
                Qt.Key_Down,
                Qt.Key_Home,
                Qt.Key_End,
                Qt.Key_PageUp,
                Qt.Key_PageDown,
            ]:
                pass
            else:
                self._editor.setFocus()
                self._editor.event(event)
                self._popup.hide()
            return consumed

        return False

    def show_completion(self, choices):
        if not choices:
            return

        pallete = self._editor.palette()
        color = pallete.color(QPalette.Disabled, QPalette.WindowText)

        self._popup.setUpdatesEnabled(False)
        self._popup.clear()

        for choice in choices:
            item = QTreeWidgetItem(self._popup)
            item.setText(0, strip_tags(choice["label"]))
            item.setData(0, Qt.UserRole, choice["lon"])
            item.setData(0, Qt.UserRole + 1, choice["lat"])
            item.setForeground(0, color)

        self._popup.setCurrentItem(self._popup.topLevelItem(0))
        self._popup.resizeColumnToContents(0)
        self._popup.setUpdatesEnabled(True)

        self._popup.move(self._editor.mapToGlobal(QPoint(0, self._editor.height())))
        self._popup.setFocus()
        self._popup.show()

    def done_completion(self):
        self._timer.stop()
        self._popup.hide()
        self._editor.setFocus()

        item = self._popup.currentItem()
        selected = None
        if item:
            label = strip_tags(item.text(0))
            lon = item.data(0, Qt.UserRole)
            lat = item.data(0, Qt.UserRole + 1)
            self._editor.setText(label)
            QMetaObject.invokeMethod(self._editor, "returnPressed")
            selected = {"label": label, "lon": lon, "lat": lat}
        else:
            selected = self.catch_coordinates(self._editor.text())
            print(self._editor.text())
        if selected:
            print(selected)
            self.finished.emit(selected)

    @staticmethod
    def catch_coordinates(text):
        if text:
            lon_lat_match = re.match('^(\d+(\.?\d+?)?),(\d+(\.?\d+?)?)$', text)
            if lon_lat_match:
                lon = float(lon_lat_match[1])
                lat = float(lon_lat_match[3])
                selected = {"label": "{},{}".format(lon, lat), "lon": lon, "lat": lat}
                return selected
        return None

    def auto_suggest(self):
        text = self._editor.text()
        if self.catch_coordinates(text):
            pass
        elif text:
            is_offline = False if QgsSettings().value("/kadas/isOffline") == "false" else True
            LOG.debug("is_offline %s" % is_offline)
            if is_offline:
                url = QgsSettings().value(
                    "search/locationofflinesearchurl",
                    "http://localhost:5000/SearchServerCh",
                )
            else:
                url = QgsSettings().value(
                    "search/locationsearchurl",
                    "https://api3.geo.admin.ch/rest/services/api/SearchServer",
                )
            url = QUrl(url)
            query = QUrlQuery()
            query.addQueryItem("sr", "2056")
            query.addQueryItem("searchText", text)
            query.addQueryItem("lang", "en")
            query.addQueryItem("type", "locations")
            query.addQueryItem("limit", "10")
            url.setQuery(query)
            LOG.debug(url)
            self._network_manager.get(QNetworkRequest(url))

    def prevent_suggest(self):
        self._timer.stop()

    def handle_network_data(self, network_reply):
        choices = []
        if network_reply.error() == QNetworkReply.NoError:
            data_raw = network_reply.readAll().data()
            try:
                data = json.loads(data_raw)
                if data.get("status") != "error":
                    for location in data["results"]:
                        attributes = location.get("attrs", {})
                        choice = {
                            "label": attributes.get("label", "Unknown label"),
                            "lon": attributes.get("lon", 0.0),
                            "lat": attributes.get("lat", 0.0),
                        }
                        choices.append(choice)
                    self.show_completion(choices)
                else:
                    error_detail = data.get("detail", "No error detail.")
                    LOG.error(data)
                    self.error.emit(error_detail)
            except json.decoder.JSONDecodeError:
                data_string = data_raw.decode("utf-8")
                LOG.error(data_string)
                self.error.emit(strip_tags(data_string))
        else:
            try:
                error_message = network_reply.readAll().data().decode("utf-8")
                self.error.emit(error_message)
                LOG.error(error_message)
            except Exception as e:
                LOG.debug(e)
                self.error.emit(network_reply.errorString())
        network_reply.deleteLater()


class AutoCompleteWidget(QLineEdit):

    finished = pyqtSignal(dict)
    error = pyqtSignal(str)

    def __init__(self, parent=None):
        super(AutoCompleteWidget, self).__init__(parent)
        self._completer = SuggestCompletion(self)
        self._completer.finished.connect(self.finished.emit)
        self._completer.error.connect(self.error.emit)
