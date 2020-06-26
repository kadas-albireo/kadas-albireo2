from PyQt5.QtWidgets import QWidget, QHBoxLayout, QToolButton 
from PyQt5.QtGui import QIcon

from qgis.core import (QgsCoordinateReferenceSystem,
                       QgsCoordinateTransform,
                       QgsProject,
                       QgsPointXY
                       )

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
        self.btnMapTool.setToolTip('Choose location on the map')
        self.btnMapTool.setIcon(QIcon(":/kadas/icons/pick"))
        self.btnMapTool.clicked.connect(self.selectPoint)
        self.layout.addWidget(self.btnMapTool)

        self.setLayout(self.layout)

        self.mapTool = PointCaptureMapTool(canvas)        
        self.mapTool.canvasClicked.connect(self.updatePoint)
        self.mapTool.complete.connect(self.pointPicked)

    def selectPoint(self):
        self.prevMapTool = self.canvas.mapTool()
        self.canvas.setMapTool(self.mapTool)

    def updatePoint(self, point, button):
        outCrs = QgsCoordinateReferenceSystem(4326)
        transform = QgsCoordinateTransform(QgsProject.instance().crs(), outCrs, QgsProject.instance())
        wgspoint = transform.transform(point)
        s = '{:.6f},{:.6f}'.format(wgspoint.x(), wgspoint.y())
        #TODO set text in search box

    def pointPicked(self):        
        self.canvas.setMapTool(self.prevMapTool)

    def valueAsPoint(self):
        #TODO geocode and return coordinates based on text in the text field, or raise WrongPlaceException
        return QgsPointXY(0,0)
