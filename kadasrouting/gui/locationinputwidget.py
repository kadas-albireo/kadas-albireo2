import json
import logging

from PyQt5.QtWidgets import QWidget, QHBoxLayout, QToolButton, QLineEdit, QCompleter
from PyQt5.QtGui import QIcon, QStandardItemModel, QStandardItem
from PyQt5.QtCore import QEventLoop, QUrl, pyqtSignal, pyqtSlot, Qt, QUrlQuery, QModelIndex
from PyQt5 import QtNetwork

from qgis.core import (
    QgsCoordinateReferenceSystem,
    QgsCoordinateTransform,
    QgsProject,
    QgsPointXY,
    QgsGpsDetector,
    QgsSettings,
)

from kadasrouting.utilities import (
    icon,
    pushWarning,
    waitcursor,
)

from kadasrouting.gui.pointcapturemaptool import PointCaptureMapTool

from kadasrouting.gui.autocompletewidget import AutoCompleteWidget

from kadas.kadasgui import (
    KadasSearchBox,
    KadasCoordinateSearchProvider,
    KadasLocationSearchProvider,
    KadasLocalDataSearchProvider,
    KadasRemoteDataSearchProvider,
    KadasWorldLocationSearchProvider,
    KadasPinSearchProvider,
    KadasSearchProvider,
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

    def __init__(self, canvas, locationSymbolPath=":/kadas/icons/pin_red"):
        QWidget.__init__(self)
        # UI
        self.canvas = canvas
        self.locationSymbolPath = locationSymbolPath
        self.layout = QHBoxLayout()
        self.layout.setMargin(0)

        self.searchBox = AutoCompleteWidget()
        self.searchBox.completer().activated[QModelIndex].connect(self.getLocation)
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
        self.locationName = ''
        self._gpsConnection = None

    def getLocation(self, modelIndex):
        label = modelIndex.data()
        lat = modelIndex.data(Qt.UserRole)
        lon = modelIndex.data(Qt.UserRole + 1)
        x = modelIndex.data(Qt.UserRole + 2)
        y = modelIndex.data(Qt.UserRole)
        LOG.debug('label selected %s' % label)
        self.setPointFromLonLat(lon, lat)

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
            self.setPoint(wgspoint)
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
            self.searchBox.disableAutoComplete()
            self.startSelectingPoint()
        else:
            self.stopSelectingPoint()
            self.searchBox.enableAutoComplete()

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
        self.pin.setFilePath(self.locationSymbolPath)
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

    def setPoint(self, point):
        self.point = point
        LOG.debug('Current point is %s' % self.point.asWkt())
        self.pointUpdated.emit(self.point)
        self.addPin()

    def setPointFromLonLat(self, lon, lat):
        self.setPoint(QgsPointXY(lon, lat))

    def setLocationName(self, name):
        self.locationName = name
