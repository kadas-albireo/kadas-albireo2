import os

from PyQt5.QtWidgets import QTextBrowser

from kadas.kadasgui import (    
    KadasPinItem,
    KadasItemPos,
    KadasMapCanvasItemManager,
    KadasPluginInterface)

from kadasrouting.utilities import pushWarning
from kadasrouting.gui.pointcapturemaptool import PointCaptureMapTool
from kadasrouting.core.optimalroutelayer import OptimalRouteLayer, NotInRouteException
from kadasrouting.gui.gps import getGpsConnection

from qgis.utils import iface

from qgis.core import (
    QgsProject,
    QgsCoordinateTransform,
    QgsCoordinateReferenceSystem
)

info_html_template = '''
<table border="0" style="border-collapse: collapse; width: 100%; height: 100%;">
<tbody>
<tr>
<td style="width: 100%; background-color: #333f4f; text-align: center;">
<p><img src="{icon}" alt="" width="100" height="100" style="display: block; margin-left: auto; margin-right: auto;" /></p>
<h3 style="text-align: center;"><span style="color: #ffffff;">{dist}<br/>{message}</span></h3>
</td>
</tr>
<tr>
<td style="width: 100%; background-color: #adb9ca; text-align: center;"><img src="{icon2}" width="32" height="32" />&nbsp;{dist2}<br/> {message2}</td>
</tr>
<tr>
<td style="width: 100%; background-color: #44546a;">
<p style="text-align: center;"><span style="color: #ffffff;">Speed {speed} km/h</span><br /><span style="color: #ffffff;">Time Left {timeleft}</span><br /><span style="color: #ffffff;">Dist Left {distleft}</span><br /><span style="color: #ffffff;">ETA {eta}</span></p>
<p style="text-align: center;"><span style="color: #ffffff;">My Position:</span><br /><span style="color: #ffffff;">{x}</span><br /><span style="color: #ffffff;">{y}</span></p>
</td>
</tr>
</tbody>
</table>
'''

error_html_template = '''
<table border="0" style="border-collapse: collapse; width: 100%;">
<tbody>
<tr>
<td style="width: 100%; background-color: #333f4f; text-align: center;">
<h3 style="text-align: center;"><span style="color: #ffffff;">{text}</span></h3>
</td>
</tr>
</tbody>
</table>
'''

class NavigationPanel(QTextBrowser):
    
    def __init__(self):
        super().__init__()
        self.setStyleSheet("background-color: #333f4f;")
        self.iface = KadasPluginInterface.cast(iface)
        self.gpsConnection = None
        self.centerPin = None

    def show(self):
        super().show()
        self.startNavigation()


    def hide(self):
        super().hide()
        self.stopNavigation()

    def updateNavigationInfo(self, gpsinfo):
        layer = iface.activeLayer()
        if isinstance(layer, OptimalRouteLayer):
            point = QgsPointXY(gpsinfo.longitude, gpsinfo.latitude)
            try:
                maneuver = layer.maneuverForPoint(point, gpsinfo.speed)                
                html = info_html_template.format(**maneuver)
                self.setHtml(html)
                self.centerPin.setPosition(KadasItemPos(point.x(), point.y()))
                iface.mapCanvas().setRotation(gpsinfo.direction)
            except NotInRouteException:
                self.setHtml(error_html_template.format(text="You are not in the route"))
        else:
            self.setHtml(error_html_template.format(text="Select a route layer for navigation"))
        
    def startNavigation(self):
        self.setHtml(error_html_template.format(text="Connecting to GPS..."))
        self.gpsConnection = getGpsConnection()
        if self.gpsConnection is None:
            self.setHtml(error_html_template.format(text="Cannot connect to GPS"))
        else:
            self.gpsConnection.statusChanged.connect(self.updateNavigationInfo)
            center = iface.mapCanvas().center()
            outCrs = QgsCoordinateReferenceSystem(4326)
            canvasCrs = iface.mapCanvas().mapSettings().destinationCrs()
            transform = QgsCoordinateTransform(canvasCrs, outCrs, QgsProject.instance())
            wgspoint = transform.transform(center)

            self.centerPin = KadasPinItem(outCrs)
            self.centerPin.setPosition(KadasItemPos(wgspoint.x(), wgspoint.y()))
            icon = os.path.join(os.path.dirname(os.path.dirname(__file__)), 
                                "icons", "navigationcenter.svg")
            self.centerPin.setup(
                icon,
                self.centerPin.anchorX(),
                self.centerPin.anchorX(),
                32,
                32,
            )
            
            KadasMapCanvasItemManager.addItem(self.centerPin)
            self.updateNavigationInfo(self.gpsConnection.currentGPSInformation())
            self.point = wgspoint
            self.iface.layerTreeView().currentLayerChanged.connect(self.currentLayerChanged)

    def currentLayerChanged(self, layer):
        self.updateNavigationInfo(self.point, None)

    
    def stopNavigation(self):        
        iface.mapCanvas().setRotation(0)
        if self.gpsConnection is not None:
            self.gpsConnection.statusChanged.disconnect(self.updateNavigationInfo)
        if self.centerPin is not None:
            KadasMapCanvasItemManager.removeItem(self.centerPin)
        self.iface.layerTreeView().currentLayerChanged.disconnect(self.currentLayerChanged)
        

