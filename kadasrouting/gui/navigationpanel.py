import os
import math
import datetime

from PyQt5 import uic

from PyQt5.QtGui import QPixmap, QTransform, QPainter

from PyQt5.QtCore import Qt, QSize

from PyQt5.QtWidgets import (
    QListWidgetItem,
    QListWidget,
    QLabel,
    QInputDialog
)

from kadas.kadasgui import (
    KadasPinItem,
    KadasItemPos,
    KadasItemLayer,
    KadasMapCanvasItemManager,
    KadasPluginInterface,
    KadasGpxWaypointItem)

from kadasrouting.utilities import formatdist
from kadasrouting.core.optimalroutelayer import OptimalRouteLayer, NotInRouteException
from kadasrouting.gui.gps import getMockupGpsConnection
from kadasrouting.core import vehicles

from qgis.utils import iface

from qgis.core import (
    QgsProject,
    QgsCoordinateTransform,
    QgsCoordinateReferenceSystem,
    QgsDistanceArea,
    QgsUnitTypes,
    QgsPointXY,
    QgsVectorLayer,
    QgsWkbTypes
)

route_html_template = '''
<table border="0" style="border-collapse: collapse; width: 100%; height: 100%;">
<tbody>
<tr>
<td style="width: 100%; background-color: #333f4f; text-align: center;">
git<p>
<img src="{icon}" alt="" width="100" height="100" style="display: block; margin-left: auto; margin-right: auto;" />
</p>
<h3 style="text-align: center;"><span style="color: #ffffff;">{dist}<br/>{message}</span></h3>
</td>
</tr>
<tr>
<td style="width: 100%; background-color: #adb9ca; text-align: center;">
<img src="{icon2}" width="32" height="32" />&nbsp;{dist2}<br/> {message2}</td>
</tr>
<tr>
<td style="width: 100%; background-color: #44546a;">
<p style="text-align: center;">
<span style="color: #ffffff;">Speed {speed} km/h</span><br />
<span style="color: #ffffff;">Time Left {timeleft}</span><br />
<span style="color: #ffffff;">Dist Left {distleft}</span><br />
<span style="color: #ffffff;">ETA {eta}</span></p>
<p style="text-align: center;"><span style="color: #ffffff;">My Position:</span><br />
<span style="color: #ffffff;">{x}</span><br />
<span style="color: #ffffff;">{y}</span></p>
</td>
</tr>
</tbody>
</table>
'''

waypoint_html_template = '''
<table border="0" style="border-collapse: collapse; width: 100%; height: 100%;">
<tbody>
<tr>
<td style="width: 100%; background-color: #44546a;">
<p style="text-align: center;">
<span style="color: yellow; font-size:14pt;">Ground Heading </span>
<span style="color: #ffffff;"> {heading:.0f}°</span><br/>
<span style="color: #adb9ca; font-size:14pt;">WP Angle </span>
<span style="color: #ffffff;"> {wpangle:.0f}°</span><br />
<span style="color: #ffffff;">Dist {distleft}</span><br />
<span style="color: #ffffff;">Speed {speed:.2f} km/h</span><br />
<span style="color: #ffffff;">ETA {eta}</span></p>
</td>
</tr>
</tbody>
</table>
'''

waypoint_name_html_template = '<h3 style="text-align: center;"><span style="color: #ffffff;">{name}</span></h3>'

waypoint_miniature_html_template = '''
<span style="color: white;">{name}<br/>
<span style="font-size:8pt;">Dist {dist} | {wpangle:.0f}°</span></span>
'''

waypoint_miniature_selected_html_template = '''
<span style="color: white; font-weight: bold; ">{name}<br/>
<span style="font-size:8pt;">Dist {dist} | {wpangle:.0f}°</span></span>
'''

message_html_template = '''
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

WIDGET, BASE = uic.loadUiType(
    os.path.join(os.path.dirname(__file__), "navigationpanel.ui")
)


def _icon_path(name):
    return os.path.join(os.path.dirname(os.path.dirname(__file__)),
                        "icons", name)


def getInstructionsToWaypoint(waypoint, gpsinfo):
    point = QgsPointXY(gpsinfo.longitude, gpsinfo.latitude)
    qgsdistance = QgsDistanceArea()
    qgsdistance.setSourceCrs(
        QgsCoordinateReferenceSystem(4326), QgsProject.instance().transformContext())
    qgsdistance.setEllipsoid(qgsdistance.sourceCrs().ellipsoidAcronym())
    wpangle = math.degrees(qgsdistance.bearing(point, waypoint))
    dist = qgsdistance.convertLengthMeasurement(
                                        qgsdistance.measureLine(waypoint, point),
                                        QgsUnitTypes.DistanceMeters)

    timeleft = dist / 1000 / gpsinfo.speed * 3600
    delta = datetime.timedelta(seconds=timeleft)
    eta = datetime.datetime.now() + delta
    eta_string = eta.strftime("%H:%M")

    return {"heading": gpsinfo.direction,
            "wpangle": wpangle,
            "distleft": formatdist(dist),
            "speed": gpsinfo.speed,
            "eta": eta_string}


class NavigationPanel(BASE, WIDGET):

    FIXED_WIDTH = 200

    def __init__(self):
        super().__init__()
        self.setupUi(self)
        self.setStyleSheet("background-color: #333f4f;")
        self.textBrowser.setStyleSheet("background-color: #adb9ca;")
        self.listWaypoints.setStyleSheet("background-color: #adb9ca;")
        self.iface = KadasPluginInterface.cast(iface)
        self.gpsConnection = None
        self.listWaypoints.setSelectionMode(QListWidget.SingleSelection)
        self.listWaypoints.currentItemChanged.connect(self.selectedWaypointChanged)
        self.listWaypoints.setSpacing(5)
        self.waypointWidgets = []
        self.optimalRoutesCache = {}

    def show(self):
        super().show()
        self.startNavigation()

    def hide(self):
        super().hide()
        self.stopNavigation()

    def updateNavigationInfo(self, gpsinfo):
        self.currentGpsInformation = gpsinfo
        if gpsinfo is None:
            self.setMessage("Cannot connect to GPS")
            return
        layer = self.iface.activeLayer()
        point = QgsPointXY(gpsinfo.longitude, gpsinfo.latitude)
        origCrs = QgsCoordinateReferenceSystem(4326)
        canvasCrs = iface.mapCanvas().mapSettings().destinationCrs()
        transform = QgsCoordinateTransform(origCrs, canvasCrs, QgsProject.instance())
        canvasPoint = transform.transform(point)
        self.centerPin.setPosition(KadasItemPos(point.x(), point.y()))
        iface.mapCanvas().setCenter(canvasPoint)
        iface.mapCanvas().setRotation(-gpsinfo.direction)
        iface.mapCanvas().refresh()

        if isinstance(layer, QgsVectorLayer) and layer.geometryType() == QgsWkbTypes.LineGeometry:
            feature = next(layer.getFeatures(), None)
            if feature:
                geom = feature.geometry()
                layer = self.getOptimalRouteLayerForGeometry(geom)

        if isinstance(layer, OptimalRouteLayer) and layer.hasRoute():
            try:
                maneuver = layer.maneuverForPoint(point, gpsinfo.speed)
            except NotInRouteException:
                self.setMessage("You are not in the route")
                return
            self.setWidgetsVisibility(False)
            html = route_html_template.format(**maneuver)
            self.textBrowser.setHtml(html)
            self.textBrowser.setFixedHeight(self.textBrowser.document().size().height())
        elif isinstance(layer, KadasItemLayer):
            waypoints = self.waypointsFromLayer(layer)
            if waypoints:
                if self.waypointLayer is None:
                    self.waypointLayer = layer
                    self.populateWaypoints(waypoints)
                else:
                    self.updateWaypoints()
                waypointItem = self.listWaypoints.currentItem() or self.listWaypoints.item(0)
                waypoint = waypointItem.point
                instructions = getInstructionsToWaypoint(waypoint, gpsinfo)
                self.setCompass(instructions["heading"], instructions["wpangle"])
                html = waypoint_html_template.format(**instructions)
                self.textBrowser.setHtml(html)
                self.textBrowser.setFixedHeight(self.textBrowser.document().size().height())
                self.labelWaypointName.setText(waypoint_name_html_template.format(name=waypointItem.name))
                self.setWidgetsVisibility(True)
            else:
                self.setMessage("Select a route or waypoint layer for navigation")
        else:
            self.setMessage("Select a route or waypoint layer for navigation")

    def getOptimalRouteLayerForGeometry(self, geom):
        wkt = geom.asWkt()
        if wkt in self.optimalRoutesCache:
            return self.optimalRoutesCache[wkt]

        name = self.iface.activeLayer().name()
        value, ok = QInputDialog.getItem(
            iface.mainWindow(),
            f"Navigation",
            f"Select Vehicle to use with layer '{name}'",
            vehicles.vehicle_reduced_names())
        if ok:
            profile, costingOptions = vehicles.options_for_vehicle_reduced(
                vehicles.vehicle_reduced_names().index(value))
            layer = OptimalRouteLayer("")
            layer.updateFromPolyline(geom.asPolyline(), profile, costingOptions)
            self.optimalRoutesCache[wkt] = layer
            return layer

    def setCompass(self, heading, wpangle):
        compassPixmap = QPixmap(_icon_path("compass.png"))
        compassPixmap = compassPixmap.scaledToWidth(self.FIXED_WIDTH)
        bearingPixmap = QPixmap(_icon_path("direction.png"))
        pixmap = QPixmap(self.FIXED_WIDTH, self.FIXED_WIDTH)
        pixmap.fill(Qt.transparent)
        painter = QPainter(pixmap)
        transform = QTransform()
        transform.translate(self.FIXED_WIDTH / 2, self.FIXED_WIDTH / 2)
        transform.rotate(-heading)
        transform.translate(-self.FIXED_WIDTH / 2, -self.FIXED_WIDTH / 2)
        painter.setTransform(transform)
        painter.drawPixmap(0, 0, self.FIXED_WIDTH, self.FIXED_WIDTH, compassPixmap)
        transform = QTransform()
        transform.translate(self.FIXED_WIDTH / 2, self.FIXED_WIDTH / 2)
        transform.rotate(wpangle-heading)
        transform.translate(-self.FIXED_WIDTH / 2, -self.FIXED_WIDTH / 2)
        painter.setTransform(transform)
        painter.drawPixmap(0, 0, self.FIXED_WIDTH, self.FIXED_WIDTH, bearingPixmap)
        painter.end()
        self.labelCompass.setPixmap(pixmap)
        self.labelCompass.resize(QSize(self.FIXED_WIDTH, self.FIXED_WIDTH))

    def updateWaypoints(self):
        for item, w in self.waypointWidgets:
            w.setWaypointText(self.currentGpsInformation)

    def selectedWaypointChanged(self, current, previous):
        for item, w in self.waypointWidgets:
            w.setIsItemSelected(current == item)
        self.updateNavigationInfo(self.currentGpsInformation)

    def waypointsFromLayer(self, layer):
        try:
            '''
            center = iface.mapCanvas().center()
            outCrs = QgsCoordinateReferenceSystem(4326)
            canvasCrs = iface.mapCanvas().mapSettings().destinationCrs()
            transform = QgsCoordinateTransform(canvasCrs, outCrs, QgsProject.instance())
            wgspoint = transform.transform(center)
            item = KadasGpxWaypointItem()
            item.addPartFromGeometry(QgsPoint(wgspoint.x() + 10, wgspoint.y() + 10))
            item.setName("Test Waypoint")
            item2 = KadasGpxWaypointItem()
            item2.addPartFromGeometry(QgsPoint(wgspoint.x() + 10, wgspoint.y() + 10))
            item2.setName("Another Waypoint")
            item3 = KadasGpxWaypointItem()
            item3.addPartFromGeometry(QgsPoint(wgspoint.x() + 10, wgspoint.y() + 10))
            item3.setName("My Waypoint")
            return [item, item2, item3]
            '''
            return [item for item in layer.items() if isinstance(item, KadasGpxWaypointItem)]
        except Exception as e:
            return []

    def populateWaypoints(self, waypoints):
        self.listWaypoints.clear()
        self.waypointWidgets = []
        for waypoint in waypoints:
            item = WaypointItem(waypoint)
            widget = WaypointItemWidget(waypoint, self.currentGpsInformation)
            self.listWaypoints.addItem(item)
            item.setSizeHint(widget.sizeHint())
            self.listWaypoints.setItemWidget(item, widget)
            self.waypointWidgets.append((item, widget))
        self.selectedWaypointChanged(self.listWaypoints.item(0), None)

    def setMessage(self, text):
        self.setWidgetsVisibility(False)
        self.textBrowser.setHtml(message_html_template.format(text=text))
        self.textBrowser.setFixedHeight(self.height())

    def setWidgetsVisibility(self, iswaypoints):
        self.labelWaypointName.setVisible(iswaypoints)
        self.labelCompass.setVisible(iswaypoints)
        self.listWaypoints.setVisible(iswaypoints)
        self.labelWaypoints.setVisible(False)  # iswaypoints)
        color = "#adb9ca" if iswaypoints else "#333f4f"
        self.textBrowser.setStyleSheet("background-color: {};".format(color))

    def startNavigation(self):
        self.centerPin = None
        self.waypointLayer = None
        self.currentGpsInformation = None

        self.setMessage("Connecting to GPS...")
        self.gpsConnection = getMockupGpsConnection()
        if self.gpsConnection is None:
            self.setMessage("Cannot connect to GPS")
        else:
            self.gpsConnection.statusChanged.connect(self.updateNavigationInfo)
            self.centerPin = KadasPinItem(QgsCoordinateReferenceSystem(4326))
            self.centerPin.setup(
                _icon_path("navigationcenter.svg"),
                self.centerPin.anchorX(),
                self.centerPin.anchorX(),
                32,
                32,
            )

            KadasMapCanvasItemManager.addItem(self.centerPin)
            self.updateNavigationInfo(self.gpsConnection.currentGPSInformation())
        self.iface.layerTreeView().currentLayerChanged.connect(self.currentLayerChanged)

    def currentLayerChanged(self, layer):
        self.waypointLayer = None
        self.updateNavigationInfo(self.currentGpsInformation)

    def stopNavigation(self):
        iface.mapCanvas().setRotation(0)
        iface.mapCanvas().refresh()
        if self.gpsConnection is not None:
            self.gpsConnection.statusChanged.disconnect(self.updateNavigationInfo)
        if self.centerPin is not None:
            KadasMapCanvasItemManager.removeItem(self.centerPin)
        self.iface.layerTreeView().currentLayerChanged.disconnect(self.currentLayerChanged)


class WaypointItem(QListWidgetItem):

    def __init__(self, waypoint):
        super().__init__(waypoint.name())
        pos = waypoint.position()
        self.point = QgsPointXY(pos.x(), pos.y())
        self.name = waypoint.name()


class WaypointItemWidget(QLabel):

    def __init__(self, waypoint, gpsinfo):
        super().__init__()
        self.setMargin(4)
        self.setStyleSheet("background-color: #44546a;")
        self.waypoint = waypoint
        self.selected = False
        self.setWaypointText(gpsinfo)

    def setWaypointText(self, gpsinfo):
        self.gpsinfo = gpsinfo
        pos = self.waypoint.position()
        point = QgsPointXY(pos.x(), pos.y())
        instructions = getInstructionsToWaypoint(point, gpsinfo)
        template = waypoint_miniature_selected_html_template if self.selected else waypoint_miniature_html_template
        self.setText(template.format(
                name=self.waypoint.name(),
                wpangle=instructions["wpangle"],
                dist=instructions["distleft"]))
        return

    def setIsItemSelected(self, selected):
        self.selected = selected
        self.setWaypointText(self.gpsinfo)
