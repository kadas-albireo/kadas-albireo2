import json
import logging

from PyQt5.QtCore import Qt, pyqtSignal, QEventLoop, pyqtSlot, QUrl, QUrlQuery
from PyQt5.QtGui import QStandardItemModel, QStandardItem
from PyQt5.QtNetwork import QNetworkAccessManager, QNetworkRequest, QNetworkReply
from PyQt5.QtWidgets import QCompleter, QLineEdit

from kadasrouting.utilities import strip_tags

LOG = logging.getLogger(__name__)


class SuggestionPlaceModel(QStandardItemModel):
    finished = pyqtSignal()
    error = pyqtSignal(str)

    def __init__(self, parent=None):
        super(SuggestionPlaceModel, self).__init__(parent)
        self._manager = QNetworkAccessManager(self)
        self._reply = None

    @pyqtSlot(str)
    def search(self, text):
        self.clear()
        if self._reply is not None:
            self._reply.abort()
        if text:
            r = self.create_request(text)
            self._reply = self._manager.get(r)
            self._reply.finished.connect(self.on_finished)
        loop = QEventLoop()
        self.finished.connect(loop.quit)
        loop.exec_()

    def create_request(self, text):
        url = QUrl("https://api3.geo.admin.ch/rest/services/api/SearchServer")
        query = QUrlQuery()
        query.addQueryItem("sr", "2056")
        query.addQueryItem("searchText", text)
        query.addQueryItem("lang", "en")
        query.addQueryItem("type", "locations")
        url.setQuery(query)
        LOG.debug(url.toString())
        request = QNetworkRequest(url)
        return request

    @pyqtSlot()
    def on_finished(self):
        reply = self.sender()
        if reply.error() == QNetworkReply.NoError:
            data = json.loads(reply.readAll().data())
            if data.get('status') != 'error':
                for location in data['results']:
                    attributes = location.get('attrs', {})
                    label = attributes.get('label', 'Unknown label')
                    # Create standard item to store the data
                    item = QStandardItem(strip_tags(label))
                    item.setData(attributes.get('lat'), Qt.UserRole)
                    item.setData(attributes.get('lon'), Qt.UserRole + 1)
                    item.setData(attributes.get('x'), Qt.UserRole + 2)
                    item.setData(attributes.get('y'), Qt.UserRole + 3)
                    item.setData(label, Qt.UserRole + 4)
                    self.appendRow(item)
            else:
                self.error.emit(data.get('detail', 'Unknown error detail'))
                LOG.debug('Error happened but request is success %s' % data.get('detail', 'Unknown error detail'))
        else:
            LOG.debug('Error happened with the request. Error code %s' % reply.error())
        self.finished.emit()
        reply.deleteLater()
        self._reply = None

class Completer(QCompleter):
    def splitPath(self, path):
        self.model().search(path)
        return super(Completer, self).splitPath(path)

class AutoCompleteWidget(QLineEdit):
    def __init__(self, parent=None):
        super(AutoCompleteWidget, self).__init__(parent)
        self._model = SuggestionPlaceModel(self)
        completer = Completer(self, caseSensitivity=Qt.CaseInsensitive)
        completer.setModel(self._model)
        self.setCompleter(completer)
