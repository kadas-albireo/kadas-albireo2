from PyQt5.QtWidgets import QTextBrowser

from qgis.utils import iface

from kadasrouting.utilities import pushWarning
from kadasrouting.gui.pointcapturemaptool import PointCaptureMapTool
from kadasrouting.core.optimalroutelayer import OptimalRouteLayer, NotInRouteException

from qgis.core import (
    QgsProject,
    QgsCoordinateTransform,
    QgsCoordinateReferenceSystem
)

class NavigationPanel(QTextBrowser):
    
    def show(self):
        super().show()
        self.startNavigation()

    def hide(self):
        super().hide()
        self.stopNavigation()

    def updateNavigationInfo(self, point, button):
        layer = iface.activeLayer()
        if isinstance(layer, OptimalRouteLayer):
            outCrs = QgsCoordinateReferenceSystem(4326)
            canvasCrs = iface.mapCanvas().mapSettings().destinationCrs()
            transform = QgsCoordinateTransform(canvasCrs, outCrs, QgsProject.instance())
            wgspoint = transform.transform(point)           
            try:
                dist, message, icon, dist2, message2 = layer.maneuverForPoint(wgspoint)
                self.setHtml(f"Distance:{dist}<br> Message:{message}<br>Distance2:{dist2}<br>Message2:{message2}<br>type:{icon}")
            except NotInRouteException:
                self.setPlainText("You are not in the route")
        else:
            self.setPlainText("Select a valid route layer for navigation")
        
    def startNavigation(self):      
        self.mapTool = PointCaptureMapTool(iface.mapCanvas())
        self.mapTool.canvasClicked.connect(self.updateNavigationInfo)
        self.prevMapTool = iface.mapCanvas().mapTool()
        iface.mapCanvas().setMapTool(self.mapTool)

    
    def stopNavigation(self):
        iface.mapCanvas().setMapTool(self.prevMapTool)
        

