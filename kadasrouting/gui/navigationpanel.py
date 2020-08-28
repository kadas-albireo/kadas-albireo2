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
                maneuver = layer.maneuverForPoint(wgspoint)
                html = info_html_template.format(**maneuver)
                self.setHtml(html)
                self.centerPin.setPosition(KadasItemPos(wgspoint.x(), wgspoint.y()))
            except NotInRouteException:
                self.setHtml(error_html_template.format(text="You are not in the route"))
        else:
            self.setHtml(error_html_template.format(text="Select a route layer for navigation"))
        
    def startNavigation(self):      
        self.mapTool = PointCaptureMapTool(iface.mapCanvas())
        self.mapTool.canvasClicked.connect(self.updateNavigationInfo)
        self.prevMapTool = iface.mapCanvas().mapTool()
        iface.mapCanvas().setMapTool(self.mapTool)

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
        self.updateNavigationInfo(center, None)
        self.point = wgspoint
        self.iface.layerTreeView().currentLayerChanged.connect(self.currentLayerChanged)

    def currentLayerChanged(self, layer):
        self.updateNavigationInfo(self.point, None)

    
    def stopNavigation(self):
        iface.mapCanvas().setMapTool(self.prevMapTool)
        KadasMapCanvasItemManager.removeItem(self.centerPin)
        self.iface.layerTreeView().currentLayerChanged.disconnect(self.currentLayerChanged)
        

