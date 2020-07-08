from PyQt5.QtWidgets import QWidget, QHBoxLayout, QToolButton, QLineEdit
from PyQt5.QtGui import QIcon
from PyQt5.QtCore import QEventLoop

import sip

from qgis.core import (
    QgsCoordinateReferenceSystem,
    QgsCoordinateTransform,
    QgsProject,
    QgsPointXY,
    QgsGpsDetector,
    QgsSettings
    )

from kadasrouting.utilities import (
    icon, 
    iconPath, 
    showMessageBox, 
    pushMessage, 
    pushWarning, 
    waitcursor
    )

from kadasrouting.gui.pointcapturemaptool import PointCaptureMapTool

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
    KadasItemPos
    )

class WrongLocationException(Exception):
    pass

class LocationInputWidget(QWidget):

    def __init__(self, canvas, locationSymbolPath = ':/kadas/icons/pin_red'):
        QWidget.__init__(self)
        self.canvas = canvas
        self.locationSymbolPath = locationSymbolPath
        self.layout = QHBoxLayout()
        self.layout.setMargin(0)
        self.searchBox = QLineEdit()
        self.searchBox.textChanged.connect(self.textChanged)
        self.layout.addWidget(self.searchBox)

        self.btnGPS = QToolButton()
        self.btnGPS.setToolTip('Get GPS location')
        self.btnGPS.setIcon(icon("gps.png"))
        self.btnGPS.clicked.connect(self.getCoordFromGPS)

        self.layout.addWidget(self.btnGPS)

        self.btnMapTool = QToolButton()
        self.btnMapTool.setCheckable(True)
        self.btnMapTool.setToolTip('Choose location on the map')
        self.btnMapTool.setIcon(QIcon(":/kadas/icons/pick"))
        self.btnMapTool.toggled.connect(self.btnMapToolClicked)
        self.layout.addWidget(self.btnMapTool)

        self.setLayout(self.layout)

        self.prevMapTool = self.canvas.mapTool()
        self.mapTool = None

        self.canvas.mapToolSet.connect(self._mapToolSet)

        self.pin = None
        self._gpsConnection = None

    def textChanged(self, text):
        self.addPin()

    def _mapToolSet(self, new, old):
        if not new == self.mapTool:
            self.btnMapTool.blockSignals(True)
            self.btnMapTool.setChecked(False)
            self.btnMapTool.blockSignals(False)

    @waitcursor
    def _getGpsConnection(self):
        if self._gpsConnection is None:
            port = str(QgsSettings().value( "/kadas/gps_port", "" ));
            gpsDetector = QgsGpsDetector(port)
            loop = QEventLoop()
            def gpsDetected(connection):
                self._gpsConnection = connection
                loop.exit()
            def gpsDetectionFailed():
                loop.exit()
            gpsDetector.detected.connect(gpsDetected)
            gpsDetector.detectionFailed.connect(gpsDetectionFailed)
            gpsDetector.advance()
            loop.exec_()
        return self._gpsConnection

    def getCoordFromGPS(self):
        connection = self._getGpsConnection()
        if connection:
            info = connection.currentGPSInformation()
            s = '{:.6f},{:.6f}'.format(info.longitude, info.latitude)
            self.searchBox.setText(s)
            self.addPin()
        else:
            pushWarning("Cannot connect to GPS")

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
        s = '{:.6f},{:.6f}'.format(wgspoint.x(), wgspoint.y())
        self.searchBox.setText(s)
        self.addPin()

    def stopSelectingPoint(self):
        """Finish selecting a point."""
        self.mapTool = self.canvas.mapTool()
        self.canvas.setMapTool(self.prevMapTool)               

    def addPin(self):
        # Remove an existing pin first
        self.removePin()
        try:
            if not self.text():
                return
            point = self.valueAsPoint()
            inCrs = QgsCoordinateReferenceSystem(4326)
            canvasCrs = self.canvas.mapSettings().destinationCrs()
            transform = QgsCoordinateTransform(inCrs, canvasCrs, QgsProject.instance())
            canvasPoint = transform.transform(point)
            self.searchBox.setStyleSheet("color: black;")
        except WrongLocationException as e:
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

    def valueAsPoint(self):
        #TODO geocode and return coordinates based on text in the text field, or raise WrongPlaceException
        try:
            lon, lat = self.searchBox.text().split(",")
            point = QgsPointXY(float(lon.strip()), float(lat.strip()))
            return point
        except:
            raise WrongLocationException(self.searchBox.text())

    def text(self):
        #TODO add getter for the searchbox text.
        return self.searchBox.text()

    def setText(self, text):
        #TODO add setter for the searchbox text. Currently searchbox doesn't publish its setText
        self.searchBox.setText(text)

    def clearSearchBox(self):
        self.setText('')

