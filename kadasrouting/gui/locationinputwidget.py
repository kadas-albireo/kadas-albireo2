from PyQt5.QtWidgets import QWidget, QHBoxLayout, QToolButton, QMessageBox
from PyQt5.QtGui import QIcon

import sip

from qgis.core import (QgsCoordinateReferenceSystem,
                       QgsCoordinateTransform,
                       QgsProject,
                       QgsPointXY
                       )

from qgis.utils import iface

from kadasrouting.utilities import icon
from kadasrouting.gui.pointcapturemaptool import PointCaptureMapTool

from kadas.kadasgui import (
    KadasSearchBox,
    KadasCoordinateSearchProvider,
    KadasLocationSearchProvider,
    KadasLocalDataSearchProvider,
    KadasRemoteDataSearchProvider,
    KadasWorldLocationSearchProvider,
    KadasPinSearchProvider)

class WrongLocationException(Exception):
    pass

class LocationInputWidget(QWidget):

    def __init__(self, canvas):
        QWidget.__init__(self)
        self.canvas = canvas
        self.layout = QHBoxLayout()
        self.layout.setMargin(0)
        self.searchBox = KadasSearchBox()
        self.searchBox.init(canvas)
        self.searchBox.addSearchProvider(KadasCoordinateSearchProvider(canvas))
        self.searchBox.addSearchProvider(KadasLocationSearchProvider(canvas))
        self.searchBox.addSearchProvider(KadasLocalDataSearchProvider(canvas))
        self.searchBox.addSearchProvider(KadasPinSearchProvider(canvas))
        self.searchBox.addSearchProvider(KadasRemoteDataSearchProvider(canvas))
        self.searchBox.addSearchProvider(KadasWorldLocationSearchProvider(canvas))
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
        self.btnMapTool.clicked.connect(self.btnMapToolClicked)
        self.layout.addWidget(self.btnMapTool)

        self.setLayout(self.layout)

        self.prevMapTool = None
        self.mapTool = None

        self.createMapTool()

    def createMapTool(self):
        self.mapTool = PointCaptureMapTool(self.canvas)
        self.mapTool.canvasClicked.connect(self.updatePoint)
        self.mapTool.complete.connect(self.stopSelectingPoint)

    def btnMapToolClicked(self):
        if self.btnMapTool.isChecked():
            self.startSelectingPoint()
        else:
            self.stopSelectingPoint()

    def startSelectingPoint(self):
        """Start selecting a point (when the map tool button is clicked)"""
        self.prevMapTool = self.canvas.mapTool()
        # For some reason, the self.mapTool object is deleted by Qt after finishing the point selection.
        # This lines below makes sure that the self.mapTool exist
        if sip.isdeleted(self.mapTool):
            self.searchBox('Map tool was destroyed, creating a new one')
            self.createMapTool()
        self.canvas.setMapTool(self.mapTool)

    def updatePoint(self, point, button):
        """When the map tool click the map canvas"""
        outCrs = QgsCoordinateReferenceSystem(4326)
        transform = QgsCoordinateTransform(QgsProject.instance().crs(), outCrs, QgsProject.instance())
        wgspoint = transform.transform(point)
        s = '{:.6f},{:.6f}'.format(wgspoint.x(), wgspoint.y())
        self.showMessageBox(s)
        #TODO set text in search box
        #TODO add point on the map canvas

    def stopSelectingPoint(self):
        """Finish selecting a point."""
        self.mapTool = self.canvas.mapTool()
        self.canvas.setMapTool(self.prevMapTool)
        self.prevMapTool = None

    def valueAsPoint(self):
        #TODO geocode and return coordinates based on text in the text field, or raise WrongPlaceException
        return QgsPointXY(0,0)

    def text(self):
        #TODO add getter for the searchbox text.
        return ''

    def setText(self):
        #TODO add setter for the searchbox text. Currently searchbox doesn't publish its setText
        pass

    def showMessageBox(self, text):
        QMessageBox.information(iface.mainWindow(),  'Log', text)
