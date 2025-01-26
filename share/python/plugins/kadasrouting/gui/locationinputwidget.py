import logging

from PyQt5.QtWidgets import QWidget, QHBoxLayout, QToolButton
from PyQt5.QtGui import QIcon
from PyQt5.QtCore import pyqtSignal

from qgis.core import (
    QgsCoordinateReferenceSystem,
    QgsCoordinateTransform,
    QgsProject,
    QgsPointXY,
)

from kadasrouting.utilities import icon, pushWarning

from kadasrouting.gui.pointcapturemaptool import PointCaptureMapTool

from kadasrouting.gui.autocompletewidget import AutoCompleteWidget

from kadas.kadasgui import (
    KadasMapCanvasItemManager,
    KadasPinItem,
    KadasItemPos,
)

from .gps import getGpsConnection

LOG = logging.getLogger(__name__)


class WrongLocationException(Exception):
    pass


class LocationInputWidget(QWidget):
    pointUpdated = pyqtSignal(QgsPointXY)

    def __init__(
        self,
        canvas,
        locationSymbolPath=":/kadas/icons/pin_red",
        pinAnchorX=0.5,
        pinAnchorY=1,
    ):
        QWidget.__init__(self)
        # UI
        self.canvas = canvas
        self.locationSymbolPath = locationSymbolPath
        # By default the anchor is 0.5, 1 which means in the middle bottom of the symbol
        self.pinAnchorX = pinAnchorX
        self.pinAnchorY = pinAnchorY
        self.layout = QHBoxLayout()
        self.layout.setMargin(0)

        self.searchBox = AutoCompleteWidget()
        self.searchBox.finished.connect(self.getLocation)
        self.searchBox.error.connect(pushWarning)
        self.layout.addWidget(self.searchBox)

        self.btnGPS = QToolButton()
        self.btnGPS.setToolTip(self.tr("Get GPS location"))
        self.btnGPS.setIcon(icon("gps.png"))
        self.btnGPS.clicked.connect(self.getCoordFromGPS)

        self.layout.addWidget(self.btnGPS)

        self.btnMapTool = QToolButton()
        self.btnMapTool.setCheckable(True)
        self.btnMapTool.setToolTip(self.tr("Choose location on the map"))
        self.btnMapTool.setIcon(QIcon(":/kadas/icons/pick"))
        self.btnMapTool.toggled.connect(self.btnMapToolClicked)
        self.layout.addWidget(self.btnMapTool)

        self.setLayout(self.layout)

        self.prevMapTool = self.canvas.mapTool()
        self.mapTool = None

        self.canvas.mapToolSet.connect(self._mapToolSet)

        self.point = None
        self.pin = None
        self.locationName = ""
        self._gpsConnection = None

    def getLocation(self, dictionary):
        label = dictionary["label"]
        lat = dictionary["lat"]
        lon = dictionary["lon"]
        self.setPointFromLonLat(lon, lat)
        self.setLocationName(label)

    def textChanged(self, text):
        self.addPin()

    def _mapToolSet(self, new, old):
        if not new == self.mapTool:
            self.btnMapTool.blockSignals(True)
            self.btnMapTool.setChecked(False)
            self.btnMapTool.blockSignals(False)

    def getCoordFromGPS(self):
        connection = getGpsConnection()
        if connection:
            info = connection.currentGPSInformation()
            pointString = "{:.6f},{:.6f}".format(info.longitude, info.latitude)
            self.searchBox.setText(pointString)
            self.setPoint(QgsPointXY(info.longitude, info.latitude))
            # TODO: Perhaps put reverse geocoding here
            self.setLocationName(pointString)
        else:
            pushWarning(self.tr("Cannot connect to GPS"))

    def createMapTool(self):
        self.mapTool = PointCaptureMapTool(self.canvas)
        self.mapTool.canvasClicked.connect(self.updatePoint)
        self.mapTool.complete.connect(self.stopSelectingPoint)

    def btnMapToolClicked(self, checked):
        if checked:
            self.startSelectingPoint()
        else:
            self.stopSelectingPoint()

    def startSelectingPoint(self):
        """Start selecting a point (when the map tool button is clicked)"""
        self.createMapTool()
        self.canvas.setMapTool(self.mapTool)

    def updatePoint(self, point, button):
        """When the map tool click the map canvas"""
        outCrs = QgsCoordinateReferenceSystem(4326)
        canvasCrs = self.canvas.mapSettings().destinationCrs()
        transform = QgsCoordinateTransform(canvasCrs, outCrs, QgsProject.instance())
        wgspoint = transform.transform(point)
        pointString = "{:.6f},{:.6f}".format(wgspoint.x(), wgspoint.y())
        self.searchBox.setText(pointString)
        self.setPoint(wgspoint)
        # TODO: Perhaps put reverse geocoding here
        self.setLocationName(pointString)

    def stopSelectingPoint(self):
        """Finish selecting a point."""
        self.mapTool = self.canvas.mapTool()
        self.canvas.setMapTool(self.prevMapTool)

    def addPin(self):
        # Remove an existing pin first
        self.removePin()
        try:
            if not self.point:
                return
            inCrs = QgsCoordinateReferenceSystem(4326)
            canvasCrs = self.canvas.mapSettings().destinationCrs()
            transform = QgsCoordinateTransform(inCrs, canvasCrs, QgsProject.instance())
            canvasPoint = transform.transform(self.point)
            self.searchBox.setStyleSheet("color: black;")
        except WrongLocationException:
            self.searchBox.setStyleSheet("color: red;")
            return

        canvasCrs = self.canvas.mapSettings().destinationCrs()
        self.pin = KadasPinItem(canvasCrs)
        self.pin.setPosition(KadasItemPos(canvasPoint.x(), canvasPoint.y()))
        self.pin.setAnchorX(self.pinAnchorX)
        self.pin.setAnchorY(self.pinAnchorY)
        self.pin.setFilePath(self.locationSymbolPath)
        LOG.debug(
            "self.pin position %s, %s"
            % (self.pin.position().x(), self.pin.position().y())
        )
        LOG.debug("pin anchor %s, %s" % (self.pin.anchorX(), self.pin.anchorY()))
        KadasMapCanvasItemManager.addItem(self.pin)

    def removePin(self):
        if self.pin:
            KadasMapCanvasItemManager.removeItem(self.pin)

    def text(self):
        # TODO add getter for the searchbox text.
        return self.searchBox.text()

    def setText(self, text):
        # TODO add setter for the searchbox text. Currently searchbox doesn't publish its setText
        self.searchBox.setText(text)

    def clearSearchBox(self):
        self.setText("")

    def deletePoint(self):
        self.point = None
        self.removePin()

    def setPoint(self, point):
        self.point = point
        LOG.debug("Current point is %s" % self.point.asWkt())
        self.pointUpdated.emit(self.point)
        self.addPin()

    def setPointFromLonLat(self, lon, lat):
        self.setPoint(QgsPointXY(lon, lat))

    def setLocationName(self, name):
        self.locationName = name
