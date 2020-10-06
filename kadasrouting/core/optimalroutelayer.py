import os
import json
import logging
import datetime

from PyQt5.QtCore import QTimer, pyqtSignal, Qt
from PyQt5.QtGui import QColor, QPen, QBrush
from PyQt5.QtWidgets import QAction

from kadas.kadasgui import (
    KadasPinItem,
    KadasItemPos,
    KadasItemLayer,
    KadasGpxRouteItem
)

from kadasrouting.utilities import (
    iconPath,
    waitcursor,
    pushWarning,
    decodePolyline6,
    formatdist
)

from kadasrouting.valhalla.client import ValhallaClient

from qgis.core import (
    QgsProject,
    QgsVectorLayer,
    QgsCoordinateReferenceSystem,
    QgsPointXY,
    QgsGeometry,
    QgsFeature,
    QgsDistanceArea,
    QgsUnitTypes
)

from kadas.kadascore import KadasPluginLayerType

LOG = logging.getLogger(__name__)

MAX_DISTANCE_FOR_NAVIGATION = 250

_icon_for_maneuver = {
    1: "direction_depart",
    2: "direction_depart_right",
    3: "direction_depart_left",
    4: "direction_arrive_straight",
    5: "direction_arrive_right",
    6: "direction_arrive_left",
    8: "direction_continue",
    9: "direction_turn_slight_right",
    10: "direction_turn_right",
    11: "direction_turn_sharp_right",
    12: "direction_uturn",
    13: "direction_uturn",
    14: "direction_turn_sharp_left",
    15: "direction_turn_left",
    16: "direction_turn_slight_left",
    17: "direction_on_ramp_straight",
    18: "direction_on_ramp_rigth",
    19: "direction_on_ramp_left",
    20: "direction_depart_right",
    21: "direction_depart_left",
    22: "direction_continue_straight",
    23: "direction_continue_right",
    24: "direction_continue_left",
    26: "direction_roundabout",
    27: "direction_roundabout",
    37: "direction_merge_right",
    38: "direction_merge_left"
    }


def icon_path_for_maneuver(maneuvertype):
    name = _icon_for_maneuver.get(maneuvertype, "dummy")
    return _icon_path(name)


def _icon_path(name):
    return os.path.join(os.path.dirname(os.path.dirname(__file__)),
                        "icons", name + ".png")


class NotInRouteException(Exception):
    pass


class RoutePointMapItem(KadasPinItem):

    hasChanged = pyqtSignal()

    def itemName(self):
        return "Route point"

    def edit(self, context, pos, settings):
        super().edit(context, pos, settings)
        self.hasChanged.emit()


class OptimalRouteLayer(KadasItemLayer):

    LAYER_TYPE = "optimalroute"

    def __init__(self, name):
        KadasItemLayer.__init__(
            self,
            name,
            QgsCoordinateReferenceSystem("EPSG:4326"),
            OptimalRouteLayer.LAYER_TYPE,
        )
        self.geom = None
        self.response = None
        self.points = []
        self.pins = []
        self.profile = None
        self.costingOptions = {}
        self.lineItem = None
        self.valhalla = ValhallaClient()
        self.timer = QTimer()
        self.timer.setSingleShot(True)
        self.timer.timeout.connect(self.updateFromPins)
        self.actionAddAsRegularLayer = QAction(self.tr("Add to project as regular layer"))
        self.actionAddAsRegularLayer.triggered.connect(self.addAsRegularLayer)

    def setResponse(self, response):
        self.response = response

    def clear(self):
        items = self.items()
        for itemId in items.keys():
            self.takeItem(itemId)
        self.pins = []
        self.maneuvers = {}

    def hasRoute(self):
        return self.geom is not None

    def pinHasChanged(self):
        self.timer.start(1000)

    @waitcursor
    def updateFromPins(self):
        try:
            for i, pin in enumerate(self.pins):
                self.points[i] = QgsPointXY(pin.position())
            response = self.valhalla.route(
                self.points, self.profile, self.costingOptions
            )
            self.computeFromResponse(response)
            self.triggerRepaint()
        except Exception as e:
            logging.error(e, exc_info=True)
            # TODO more fine-grained error control
            pushWarning(self.tr("Could not compute route"))
            logging.error("Could not compute route")

    @waitcursor
    def updateFromPolyline(self, polyline, profile, costingOptions):
        try:
            response = self.valhalla.mapmatching(polyline, profile, costingOptions)
            self.computeFromResponse(response)
        except Exception as e:
            LOG.warning(e)
            pushWarning(self.tr("Could not compute route from polyline"))

    @waitcursor
    def updateRoute(self, points, profile, costingOptions):
        response = self.valhalla.route(points, profile, costingOptions)
        self.costingOptions = costingOptions
        self.profile = profile
        self.points = points
        self.computeFromResponse(response)
        self.triggerRepaint()

    def computeFromResponse(self, response):        
        if response is None:
            return
        epsg4326 = QgsCoordinateReferenceSystem("EPSG:4326")
        self.clear()
        self.response = response
        response_mini = response["trip"]
        coordinates = []
        self.duration = 0
        self.distance = 0
        for leg in response_mini["legs"]:
            leg_coordinates = [list(reversed(coord)) for coord in decodePolyline6(leg["shape"])]
            coordinates.extend(leg_coordinates)
            qgis_leg_coords = [QgsPointXY(x, y) for x, y in leg_coordinates]
            geom = QgsGeometry.fromPolylineXY(qgis_leg_coords)
            self.maneuvers[geom] = leg["maneuvers"]
            self.duration += leg["summary"]["time"]
            self.distance += round(leg["summary"]["length"], 3)
        qgis_coords = [QgsPointXY(x, y) for x, y in coordinates]
        self.geom = QgsGeometry.fromPolylineXY(qgis_coords)
        self.lineItem = KadasGpxRouteItem()
        self.lineItem.addPartFromGeometry(self.geom.constGet())
        self.lineItem.setName("route")
        self.lineItem.setNumber("1")
        # Format string for duration
        duration_hour = int(self.duration) // 3600
        duration_minute = (int(self.duration) % 3600) // 60
        formatted_hour = (
            str(duration_hour) if duration_hour >= 10 else "0%d" % duration_hour
        )
        formatted_minute = (
            str(duration_minute) if duration_minute >= 10 else "0%d" % duration_minute
        )
        tooltip = self.tr("Distance: {distance} km<br/>Time: {formatted_hour}h{formatted_minute}").format(
            distance=self.distance,
            formatted_hour=formatted_hour,
            formatted_minute=formatted_minute
        )
        self.lineItem.setTooltip(tooltip)
        # Line color: 005EFF
        line_color = QColor(0, 94, 255)
        self.lineItem.setOutline(QPen(line_color, 5))
        self.lineItem.setFill(QBrush(line_color, Qt.SolidPattern))

        self.addItem(self.lineItem)
        for i, pt in enumerate(self.points):
            pin = RoutePointMapItem(epsg4326)
            pin.setPosition(KadasItemPos(pt.x(), pt.y()))
            if i == 0:
                pin.setFilePath(iconPath("pin_origin.svg"))
                pin.setName(self.tr("Origin Point"))
            elif i == len(self.points) - 1:
                pin.setFilePath(iconPath("pin_destination.svg"))
                pin.setName(self.tr("Destination Point"))
            else:
                pin.setup(
                    ":/kadas/icons/waypoint", pin.anchorX(), pin.anchorX(), 32, 32
                )
                pin.setName(self.tr("Waypoint {point_index}").format(point_index=i))
            pin.hasChanged.connect(self.pinHasChanged)
            self.pins.append(pin)
            self.addItem(pin)

    def maneuverForPoint(self, pt, speed):
        min_dist = MAX_DISTANCE_FOR_NAVIGATION
        closest_leg = None
        closest_segment = None
        qgsdistance = QgsDistanceArea()
        qgsdistance.setSourceCrs(
            QgsCoordinateReferenceSystem(4326),
            QgsProject.instance().transformContext())
        qgsdistance.setEllipsoid(qgsdistance.sourceCrs().ellipsoidAcronym())

        legs = list(self.maneuvers.keys())
        for i, line in enumerate(legs):
            _, _pt, segment, _ = line.closestSegmentWithContext(pt)
            dist = qgsdistance.convertLengthMeasurement(
                        qgsdistance.measureLine(pt, _pt),
                        QgsUnitTypes.DistanceMeters)
            if dist < min_dist:
                closest_leg = line
                # FIXME: commented line below is never used
                # next_leg = None if i == len(legs) - 1 else legs[i + 1]
                closest_segment = segment
                closest_point = _pt
                min_dist = dist

        if closest_leg is not None:
            leg_points = closest_leg.asPolyline()
            maneuvers = self.maneuvers[closest_leg]
            for i, maneuver in enumerate(maneuvers[:-1]):
                if (maneuver["begin_shape_index"] < closest_segment
                        and maneuver["end_shape_index"] >= closest_segment):
                    points = [closest_point]
                    points.extend(leg_points[closest_segment:maneuver["end_shape_index"]])
                    distance_to_next = qgsdistance.convertLengthMeasurement(
                                                qgsdistance.measureLine(points),
                                                QgsUnitTypes.DistanceMeters)

                    message = maneuvers[i + 1]['instruction']
                    if i == len(maneuvers) - 2:
                        distance_to_next2 = None
                        message2 = ""
                        icon2 = _icon_path("transparentpixel")
                    else:
                        next_maneuver = maneuvers[i + 2]
                        distance_to_next2 = maneuvers[i + 1]['length'] * 1000
                        message2 = next_maneuver['instruction']
                        icon2 = icon_path_for_maneuver(maneuvers[i + 2]["type"])

                    icon = icon_path_for_maneuver(maneuvers[i + 1]["type"])

                    time_to_next = distance_to_next / 1000 / speed * 3600
                    maneuvers_ahead = maneuvers[i:]
                    timeleft = time_to_next + sum([m["time"] for m in maneuvers_ahead])
                    distanceleft = distance_to_next + sum([m["length"] for m in maneuvers_ahead]) * 1000

                    delta = datetime.timedelta(seconds=timeleft)
                    timeleft_string = ":".join(str(delta).split(":")[:-1])
                    eta = datetime.datetime.now() + delta
                    eta_string = eta.strftime("%H:%M")

                    maneuver = dict(dist=formatdist(distance_to_next), message=message, icon=icon,
                                    dist2=formatdist(distance_to_next2), message2=message2, icon2=icon2,
                                    speed=speed, timeleft=timeleft_string,
                                    distleft=formatdist(distanceleft),
                                    raw_distleft=distanceleft,
                                    eta=eta_string, x=closest_point.x(), y=closest_point.y())
                    return maneuver

        raise NotInRouteException()

    def layerTypeKey(self):
        return OptimalRouteLayer.LAYER_TYPE

    def readXml(self, node, context):
        element = node.toElement()
        response = json.loads(element.attribute("response"))
        points = json.loads(element.attribute("points"))
        self.points = [QgsGeometry.fromWkt(wkt).asPoint() for wkt in points]
        self.costingOptions = json.loads(element.attribute("costingOptions"))
        self.profile = element.attribute("profile")
        self.computeFromResponse(response)
        return True

    def writeXml(self, node, doc, context):
        KadasItemLayer.writeXml(self, node, doc, context)
        element = node.toElement()
        # write plugin layer type to project  (essential to be read from project)
        element.setAttribute("type", "plugin")
        element.setAttribute("name", self.layerTypeKey())
        element.setAttribute("response", json.dumps(self.response))
        element.setAttribute("points", json.dumps([pt.asWkt() for pt in self.points]))
        element.setAttribute("profile", self.profile)
        element.setAttribute("costingOptions", json.dumps(self.costingOptions))
        return True

    def addAsRegularLayer(self):
        layer = QgsVectorLayer(
            "LineString?crs=epsg:4326&field=id:integer&field=distance:double&field=duration:double",
            self.name(), "memory"
        )
        pr = layer.dataProvider()
        feature = QgsFeature()
        feature.setAttributes([1, self.distance, self.duration])
        feature.setGeometry(self.geom)
        pr.addFeatures([feature])
        layer.updateExtents()
        QgsProject.instance().addMapLayer(layer)


class OptimalRouteLayerType(KadasPluginLayerType):
    def __init__(self):
        KadasPluginLayerType.__init__(self, OptimalRouteLayer.LAYER_TYPE)

    def createLayer(self):
        return OptimalRouteLayer("")

    def showLayerProperties(self, layer):
        return True

    def addLayerTreeMenuActions(self, menu, layer):
        menu.addAction(layer.actionAddAsRegularLayer)
