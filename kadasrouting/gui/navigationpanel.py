import os
import math
import datetime
import logging

from PyQt5 import uic

from PyQt5.QtGui import (
    QPixmap,
    QTransform,
    QPainter,
    QColor
)

from PyQt5.QtCore import (
    Qt,
    QSize,
    QSettings,
    QTimer
)

from PyQt5.QtWidgets import (
    QListWidgetItem,
    QListWidget,
    QLabel,
    QInputDialog
)

from qgis.utils import iface

from qgis.core import (
    QgsProject,
    QgsCoordinateTransform,
    QgsCoordinateReferenceSystem,
    QgsDistanceArea,
    QgsUnitTypes,
    QgsPointXY,
    QgsVectorLayer,
    QgsWkbTypes,
    QgsGeometry
)

from qgis.gui import QgsRubberBand

from kadas.kadasgui import (
    KadasPinItem,
    KadasItemPos,
    KadasItemLayer,
    KadasMapCanvasItemManager,
    KadasPluginInterface,
    KadasGpxWaypointItem)

from kadasrouting.utilities import formatdist, pushMessage, iconPath
from kadasrouting.core.optimalroutelayer import OptimalRouteLayer, NotInRouteException
from kadasrouting.gui.gps import getGpsConnection
from kadasrouting.core import vehicles
from kadasrouting.utilities import tr

LOG = logging.getLogger(__name__)

route_html_template = '''
<table border="0" style="border-collapse: collapse; width: 100%; height: 100%;">
<tbody>
<tr>
<td style="width: 100%; background-color: #333f4f; text-align: center;">
<p>
<img src="{icon}" alt="" width="100" height="100" style="display: block; margin-left: auto; margin-right: auto;" />
</p>
<h2 style="text-align: center;"><span style="color: #ffffff;">{dist}<br/>{message}</span></h>
</td>
</tr>
<tr>
<td style="width: 100%; background-color: #adb9ca; text-align: left; font-size:12pt">''' + tr('Then') + '''
<img src="{icon2}" width="32" height="32" />&nbsp;{dist2}<br/> {message2}</td>
</tr>
<tr>
<td style="width: 100%; background-color: #44546a;">
<p style="text-align: left;">
<span style="color: #ffffff; font-size:15pt">''' + tr('Speed') + ''' {speed} km/h</span><br />
<span style="color: #ffffff; font-size:15pt">''' + tr('Time Left') + ''' {timeleft}</span><br />
<span style="color: #ffffff; font-size:15pt">''' + tr('Dist Left') + ''' {distleft}</span><br />
<span style="color: #ffffff; font-size:15pt">''' + tr('ETA') + ''' {eta}</span></p>
<p style="text-align: left;"><span style="color: #ffffff; font-size:15pt">''' + tr('My Position:') + '''</span><br />
<span style="color: #ffffff;">{displayed_point}</span></p>
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
<span style="color: yellow; font-size:14pt;">''' + tr('Ground Heading') + ''' </span>
<span style="color: #ffffff;"> {heading:.0f}°</span><br/>
<span style="color: #adb9ca; font-size:14pt;">''' + tr('WP Angle') + ''' </span>
<span style="color: #ffffff;"> {wpangle:.0f}°</span><br />
<span style="color: #ffffff;">''' + tr('Dist') + ''' {distleft}</span><br />
<span style="color: #ffffff;">''' + tr('Speed') + ''' {speed:.2f} km/h</span><br />
<span style="color: #ffffff;">''' + tr('ETA') + ''' {eta}</span></p>
</td>
</tr>
</tbody>
</table>
'''

waypoint_name_html_template = '<h3 style="text-align: center;"><span style="color: #ffffff;">{name}</span></h3>'

waypoint_miniature_html_template = '''
<span style="color: white;">{name}<br/>
<span style="font-size:8pt;">''' + tr('Dist') + ''' {dist} | {wpangle:.0f}°</span></span>
'''

waypoint_miniature_selected_html_template = '''
<span style="color: white; font-weight: bold; ">{name}<br/>
<span style="font-size:8pt;">''' + tr('Dist') + ''' {dist} | {wpangle:.0f}°</span></span>
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
            "raw_distleft": dist,
            "speed": gpsinfo.speed,
            "eta": eta_string}


class NavigationPanel(BASE, WIDGET):

    FIXED_WIDTH = 200
    WARNING_DISTANCE = 200

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

        self.timer = QTimer()

        self.rubberband = QgsRubberBand(iface.mapCanvas(), QgsWkbTypes.LineGeometry)
        self.rubberband.setStrokeColor(QColor(150, 0, 0))
        self.rubberband.setWidth(2)

        self.chkShowWarnings.setChecked(True)
        self.warningShown = False
        self.iface.messageBar().widgetRemoved.connect(self.setWarningShownOff)
        self.labelConfigureWarnings.linkActivated.connect(self.configureWarnings)

    def configureWarnings(self, url):
        threshold = QSettings().value("kadasrouting/warningThreshold", self.WARNING_DISTANCE, type=int)
        value, ok = QInputDialog.getInt(
            self.iface.mainWindow(),
            self.tr("Navigation"),
            self.tr("Set threshold for warnings (meters)"),
            threshold)
        if ok:
            QSettings().setValue("kadasrouting/warningThreshold", value)

    def setWarningShownOff(self):
        self.warningShown = False

    def show(self):
        super().show()
        self.startNavigation()

    def hide(self):
        super().hide()
        self.stopNavigation()

    def updateNavigationInfo(self):
        try:
            gpsinfo = self.gpsConnection.currentGPSInformation()
        except RuntimeError:
            # if the GPS is closed in KADAS main interface, stop the navigation
            self.stopNavigation()
            return
        if gpsinfo is None:
            self.setMessage(self.tr("Cannot connect to GPS"))
            return
        layer = self.iface.activeLayer()
        LOG.debug('Debug: type(layer) = {}'.format(type(layer)))
        point = QgsPointXY(gpsinfo.longitude, gpsinfo.latitude)
        origCrs = QgsCoordinateReferenceSystem(4326)
        canvasCrs = self.iface.mapCanvas().mapSettings().destinationCrs()
        transform = QgsCoordinateTransform(origCrs, canvasCrs, QgsProject.instance())
        canvasPoint = transform.transform(point)
        self.centerPin.setPosition(KadasItemPos(point.x(), point.y()))
        self.iface.mapCanvas().setCenter(canvasPoint)
        # stop rotating the map like a crazy when the user is almost still,
        # i.e. rotate only if we move faster than 1m/s
        if gpsinfo.speed > 1.0:
            self.iface.mapCanvas().setRotation(-gpsinfo.direction)
            self.centerPin.setAngle(0)
        self.iface.mapCanvas().refresh()
        self.rubberband.reset(QgsWkbTypes.LineGeometry)

        if isinstance(layer, QgsVectorLayer) and layer.geometryType() == QgsWkbTypes.LineGeometry:
            feature = next(layer.getFeatures(), None)
            if feature:
                geom = feature.geometry()
                layer = self.getOptimalRouteLayerForGeometry(geom)
                if layer is not None:
                    rubbergeom = QgsGeometry(layer.geom)
                    rubbergeom.transform(transform)
                    self.rubberband.setToGeometry(rubbergeom)

        if isinstance(layer, OptimalRouteLayer) and layer.hasRoute():
            try:
                maneuver = layer.maneuverForPoint(point, gpsinfo.speed)
                LOG.debug(maneuver)
            except NotInRouteException:
                self.setMessage(self.tr("You are not on the route"))
                return
            self.setWidgetsVisibility(False)
            html = route_html_template.format(**maneuver)
            self.textBrowser.setHtml(html)
            self.textBrowser.setFixedHeight(self.textBrowser.document().size().height())
            self.setWarnings(maneuver["raw_distleft"])
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
                self.setWarnings(instructions["raw_distleft"])
            else:
                self.setMessage(self.tr("Select a route or waypoint layer for navigation"))
        else:
            self.setMessage(self.tr("Select a route or waypoint layer for navigation"))

    def setWarnings(self, dist):
        threshold = QSettings().value("kadasrouting/warningThreshold", self.WARNING_DISTANCE, type=int)
        if (self.chkShowWarnings.isChecked() and not self.warningShown and dist < threshold):
            pushMessage(self.tr("In {dist} meters you will arrive at your destination").format(dist=int(dist)))
            self.warningShown = True

    def getOptimalRouteLayerForGeometry(self, geom):
        wkt = geom.asWkt()
        if wkt in self.optimalRoutesCache:
            return self.optimalRoutesCache[wkt]

        name = self.iface.activeLayer().name()
        value, ok = QInputDialog.getItem(
            self.iface.mainWindow(),
            self.tr("Navigation"),
            self.tr("Select Vehicle to use with layer '{name}'").format(name=name),
            vehicles.vehicle_reduced_names())
        if ok:
            profile, costingOptions = vehicles.options_for_vehicle_reduced(
                vehicles.vehicle_reduced_names().index(value))
            layer = OptimalRouteLayer("")
            try:
                if geom.isMultipart():
                    polyline = geom.asMultiPolyline()
                    line = polyline[0]
                else:
                    line = geom.asPolyline()
                layer.updateFromPolyline(line, profile, costingOptions)
                self.optimalRoutesCache[wkt] = layer
                return layer
            except Exception:
                return

    def setCompass(self, heading, wpangle):
        compassPixmap = QPixmap(iconPath("compass.png"))
        compassPixmap = compassPixmap.scaledToWidth(self.FIXED_WIDTH)
        bearingPixmap = QPixmap(iconPath("direction.png"))
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
            w.setWaypointText(self.gpsConnection.currentGPSInformation())

    def selectedWaypointChanged(self, current, previous):
        for item, w in self.waypointWidgets:
            w.setIsItemSelected(current == item)
        self.warningShown = False
        self.updateNavigationInfo()

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
            LOG.warning(e)
            return []

    def populateWaypoints(self, waypoints):
        self.listWaypoints.clear()
        self.waypointWidgets = []
        for waypoint in waypoints:
            item = WaypointItem(waypoint)
            widget = WaypointItemWidget(waypoint, self.gpsConnection.currentGPSInformation())
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
        self.warningShown = False

        self.setMessage(self.tr("Connecting to GPS..."))
        self.gpsConnection = getGpsConnection()
        if self.gpsConnection is None:
            self.setMessage(self.tr("Cannot connect to GPS"))
        else:
            self.centerPin = KadasPinItem(QgsCoordinateReferenceSystem(4326))
            self.centerPin.setup(
                iconPath("navigationcenter.svg"),
                self.centerPin.anchorX(),
                self.centerPin.anchorX(),
                32,
                32,
            )
            # For some reason the first time shown, the direction of the center pin is following the map canvas.
            # The line below is needed to neutralize the direction of the center pin (to make it points up)
            self.centerPin.setAngle(-self.gpsConnection.currentGPSInformation().direction)
            KadasMapCanvasItemManager.addItem(self.centerPin)
            self.updateNavigationInfo()
            self.timer.start(1000)
            self.timer.timeout.connect(self.updateNavigationInfo)
        self.iface.layerTreeView().currentLayerChanged.connect(self.currentLayerChanged)

    def currentLayerChanged(self, layer):
        self.waypointLayer = None
        self.warningShown = False
        self.updateNavigationInfo()

    def stopNavigation(self):
        if self.gpsConnection is not None:
            try:
                self.timer.timeout.disconnect(self.updateNavigationInfo)
                self.timer.stop()
            except TypeError as e:
                LOG.debug(e)
        try:
            if self.centerPin is not None:
                KadasMapCanvasItemManager.removeItem(self.centerPin)
        except Exception:
            # centerPin might have been deleted
            pass
        try:
            self.iface.layerTreeView().currentLayerChanged.disconnect(self.currentLayerChanged)
        except TypeError as e:
            LOG.debug(e)
        # Finally, reset everything
        self.rubberband.reset(QgsWkbTypes.LineGeometry)
        self.iface.mapCanvas().setRotation(0)
        self.iface.mapCanvas().refresh()


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
