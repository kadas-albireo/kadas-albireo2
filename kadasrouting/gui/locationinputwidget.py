from PyQt5.QtWidgets import QWidget, QHBoxLayout, QToolButton, QMessageBox, QLineEdit
from PyQt5.QtGui import QIcon

import sip

from qgis.core import (
    QgsCoordinateReferenceSystem,
    QgsCoordinateTransform,
    QgsProject,
    QgsPointXY
    )

from qgis.utils import iface

from kadasrouting.utilities import icon, iconPath
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
        self.layout.addWidget(self.searchBox)

        self.btnGPS = QToolButton()
        # Disable GPS buttons for now
        self.btnGPS.setEnabled(False)
        self.btnGPS.setToolTip('Get GPS location')
        self.btnGPS.setIcon(icon("gps.png"))

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

    def _mapToolSet(self, new, old):
        if not new == self.mapTool:
            self.btnMapTool.blockSignals(True)
            self.btnMapTool.setChecked(False)
            self.btnMapTool.blockSignals(False)

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
        self.addPin(point)

    def stopSelectingPoint(self):
        """Finish selecting a point."""
        self.mapTool = self.canvas.mapTool()
        self.canvas.setMapTool(self.prevMapTool)               

    def addPin(self, point):
        # Remove an existing pin first
        self.removePin()
        canvasCrs = self.canvas.mapSettings().destinationCrs()
        self.pin = KadasPinItem(canvasCrs)
        self.pin.setPosition(KadasItemPos(point.x(), point.y()))
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

    def showMessageBox(self, text):
        QMessageBox.information(iface.mainWindow(),  'Log', text)
