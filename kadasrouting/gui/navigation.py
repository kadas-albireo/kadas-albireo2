from qgis.utils import iface 

from kadasrouting.utilities import pushWarning
from kadasrouting.gui.pointcapturemaptool import PointCaptureMapTool
from kadasrouting.core.optimalroutelayer import OptimalRouteLayer

from qgis.core import (
    QgsProject,
    QgsCoordinateTransform,
    QgsCoordinateReferenceSystem
)

class Navigation():

    def activateNavigation(self):
        self.mapTool = PointCaptureMapTool(iface.mapCanvas())
        self.mapTool.canvasClicked.connect(self.showNavigationInfo)        
        self.prevMapTool = iface.mapCanvas().mapTool()
        iface.mapCanvas().setMapTool(self.mapTool)

    def deactivateNavigation(self):
        iface.mapCanvas().setMapTool(self.prevMapTool)

    def showNavigationInfo(self, point, button):
        outCrs = QgsCoordinateReferenceSystem(4326)
        canvasCrs = iface.mapCanvas().mapSettings().destinationCrs()
        transform = QgsCoordinateTransform(canvasCrs, outCrs, QgsProject.instance())
        wgspoint = transform.transform(point)
        layer = iface.activeLayer()
        if isinstance(layer, OptimalRouteLayer):
            message = layer.maneuverForPoint(wgspoint)
            pushWarning(message)
        else:
            pass