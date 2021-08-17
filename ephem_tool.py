from datetime import datetime, timezone
import ephem
import math
import os

from qgis.PyQt import uic
from qgis.PyQt.QtCore import *
from qgis.PyQt.QtGui import *
from qgis.PyQt.QtWidgets import *
from qgis.core import *
from qgis.gui import *
from kadas.kadasgui import *

from .ui_EphemToolWidget import Ui_EphemToolWidget

class EphemTool(QgsMapTool):

    def __init__(self, iface):
        QgsMapTool.__init__(self, iface.mapCanvas())

        self.iface = iface
        self.setCursor(Qt.ArrowCursor)
        self.widget = EphemToolWidget(self.iface)
        self.widget.close.connect(self.close)

    def activate(self):
        self.widget.setVisible(True)
        self.pin = KadasSymbolItem(self.iface.mapCanvas().mapSettings().destinationCrs())
        self.pin.setup( ":/kadas/icons/pin_blue", 0.5, 1.0 );
        self.pinAdded = False
        QgsMapTool.activate(self)

    def deactivate(self):
        self.widget.setVisible(False)
        if self.pinAdded:
            KadasMapCanvasItemManager.removeItem(self.pin)
            self.pinAdded = False
        self.pin = None
        QgsMapTool.deactivate(self)

    def close(self):
        self.iface.mapCanvas().unsetMapTool(self)

    def canvasReleaseEvent(self, event):
        if event.button() == Qt.LeftButton:
            self.positionPicked(self.toMapCoordinates(event.pos()))
        elif event.button() == Qt.RightButton:
            self.iface.mapCanvas().unsetMapTool(self)

    def positionPicked(self, pos):
        self.pin.setPosition(KadasItemPos.fromPoint(pos))
        if not self.pinAdded:
            KadasMapCanvasItemManager.addItem(self.pin)
            self.pinAdded = True
        mapCrs = self.iface.mapCanvas().mapSettings().destinationCrs()
        wgsCrs = QgsCoordinateReferenceSystem("EPSG:4326")
        crst = QgsCoordinateTransform(mapCrs, wgsCrs, QgsProject.instance())
        self.widget.setPos(crst.transform(pos))
        self.widget.recompute()

class EphemToolWidget(KadasBottomBar):

    close = pyqtSignal()

    def __init__(self, iface):
        KadasBottomBar.__init__(self, iface.mapCanvas())

        self.iface = iface

        self.setLayout(QHBoxLayout())
        self.layout().setSpacing(10)

        base = QWidget()
        self.ui = Ui_EphemToolWidget()
        self.ui.setupUi(base)
        self.layout().addWidget(base)

        closeButton = QPushButton()
        closeButton.setSizePolicy(QSizePolicy.Preferred, QSizePolicy.Preferred)
        closeButton.setIcon(QIcon(":/kadas/icons/close"))
        closeButton.setToolTip(self.tr("Close"))
        closeButton.clicked.connect(self.close)
        self.layout().addWidget(closeButton)
        self.layout().setAlignment(closeButton, Qt.AlignTop)

        self.ui.dateTimeEdit.setDateTime(QDateTime.currentDateTime())
        self.ui.dateTimeEdit.dateTimeChanged.connect(self.recompute)
        self.ui.tabWidgetOutput.setEnabled(False)

        self.pos = None

    def getTimestamp(self):
        return self.ui.dateTimeEdit.dateTime().toSecsSinceEpoch()

    def setPos(self, pos):
        self.pos = pos

    def recompute(self):
        if not self.pos:
            return

        self.ui.tabWidgetOutput.setEnabled(True)
        font = self.ui.labelPositionValue.font()
        font.setItalic(False)
        self.ui.labelPositionValue.setFont(font)
        x_str = QgsCoordinateFormatter.formatX(self.pos.x(), QgsCoordinateFormatter.FormatDegreesMinutesSeconds, 1)
        y_str = QgsCoordinateFormatter.formatY(self.pos.y(), QgsCoordinateFormatter.FormatDegreesMinutesSeconds, 1)
        self.ui.labelPositionValue.setText(y_str + " " + x_str)

        home = ephem.Observer()
        home.lat = str(self.pos.y())
        home.lon = str(self.pos.x())
        home.date = ephem.Date(datetime.fromtimestamp(self.getTimestamp(), timezone.utc))

        sun = ephem.Sun()
        sun.compute(home)
        self.ui.labelAzimuthElevationValue.setText("%s %s" % (self.formatDMS(sun.az), self.formatDMS(sun.alt, True)))

        moon = ephem.Moon()
        moon.compute(home)
        self.ui.labelMoonAzimuthElevationValue.setText("%s %s" % (self.formatDMS(moon.az), self.formatDMS(moon.alt, True)))

        if sun.alt >= 0:
            prev_sunrise = home.previous_rising(sun)
            self.ui.labelSunRiseValue.setText(self.dateFromEphemDate(prev_sunrise))
        else:
            next_sunrise = home.next_rising(sun)
            self.ui.labelSunRiseValue.setText(self.dateFromEphemDate(next_sunrise))
        next_sunset = home.next_setting(sun)
        self.ui.labelSunSetValue.setText(self.dateFromEphemDate(next_sunset))
        next_transit = home.next_transit(sun)
        if next_transit < next_sunset:
            self.ui.labelZenithValue.setText(self.dateFromEphemDate(next_transit))
        else:
            self.ui.labelZenithValue.setText(self.dateFromEphemDate(home.previous_transit(sun)))

        if moon.alt >= 0:
            prev_moonrise = home.previous_rising(moon)
            self.ui.labelMoonRiseValue.setText(self.dateFromEphemDate(prev_moonrise))
        else:
            next_moonrise = home.next_rising(moon)
            self.ui.labelMoonRiseValue.setText(self.dateFromEphemDate(next_moonrise))
        next_moonset = home.next_setting(moon)
        self.ui.labelMoonSetValue.setText(self.dateFromEphemDate(next_moonset))

        moon_image_suffix = math.floor(round(moon.phase/12.5)*12.5)
        self.ui.labelMoonPhaseIcon.setPixmap(QPixmap(":/plugins/Ephem/icons/moon_%d.svg" % moon_image_suffix))
        self.ui.labelMoonPhaseValue.setText("%.2f%%" % moon.phase)

    def dateFromEphemDate(self, val):
        d = ephem.to_timezone(val, ephem.UTC)
        return QDateTime.fromSecsSinceEpoch(round(d.timestamp())).toString("hh:mm")

    def formatDMS(self, val, sign=False):
        strval = str(val)
        if sign and strval[0] != "-":
            strval = "+" + strval
        parts = strval.split(":")
        if len(parts) == 3:
            return parts[0] + "°" + parts[1] + "'" + parts[2] + "\"";
        else:
            return strval
