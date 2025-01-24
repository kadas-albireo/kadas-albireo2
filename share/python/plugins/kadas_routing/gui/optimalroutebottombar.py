import os
import logging

from PyQt5 import uic
from PyQt5.QtGui import QIcon
from PyQt5.QtWidgets import QWidget
from kadas.kadasgui import (
    KadasPinItem,
    KadasItemPos,
    KadasMapCanvasItemManager,
)
from kadas_routing.gui.locationinputwidget import LocationInputWidget
from kadas_routing.utilities import iconPath

from qgis.core import QgsCoordinateReferenceSystem

from kadas_routing.gui.valhallaroutebottombar import ValhallaRouteBottomBar

WIDGET, BASE = uic.loadUiType(
    os.path.join(os.path.dirname(__file__), "optimalroutebottombar.ui")
)

LOG = logging.getLogger(__name__)


class OptimalRouteBottomBar(ValhallaRouteBottomBar, WIDGET):
    def __init__(self, canvas, action, plugin):
        self.default_layer_name = "Route"
        super().__init__(canvas, action, plugin)
        self.btnAddWaypoints.setIcon(QIcon(":/kadas/icons/add"))
        self.btnAddWaypoints.setToolTip(self.tr("Add waypoint"))
        self.waypointsSearchBox = LocationInputWidget(
            canvas, locationSymbolPath=iconPath("pin_bluegray.svg")
        )
        self.groupBox.layout().addWidget(self.waypointsSearchBox, 0, 0)
        self.btnAddWaypoints.clicked.connect(self.addWaypoints)
        self.btnAreasToAvoidFromCanvas.toggled.connect(self.setPolygonDrawingMapTool)
            
    def clearPoints(self):
        self.waypointsSearchBox.clearSearchBox()
        self.waypoints = []
        self.lineEditWaypoints.clear()
        for waypointPin in self.waypointPins:
            KadasMapCanvasItemManager.removeItem(waypointPin)
        self.waypointPins = []
        super().clearPoints()

    def addWaypoints(self):
        """Add way point to the list of way points"""
        if self.waypointsSearchBox.text() == "":
            return
        waypoint = self.waypointsSearchBox.point
        self.waypoints.append(waypoint)
        if self.lineEditWaypoints.text() == "":
            self.lineEditWaypoints.setText(self.waypointsSearchBox.text())
        else:
            self.lineEditWaypoints.setText(
                self.lineEditWaypoints.text() + ";" + self.waypointsSearchBox.text()
            )
        self.waypointsSearchBox.clearSearchBox()
        # Remove way point pin from the location input widget
        self.waypointsSearchBox.removePin()
        # Create/add new waypoint pin for the waypoint
        self.addWaypointPin(waypoint)

    def reverse(self):
        super().reverse()
        # Reverse waypoints' order
        self.waypoints.reverse()
        self.waypointPins.reverse()
        # Reverse the text on the line edit
        self.lineEditWaypoints.setText(
            ";".join(reversed(self.lineEditWaypoints.text().split(";")))
        )

    def addWaypointPin(self, waypoint):
        """Create a new pin for a waypoint with its symbology"""
        # Create pin with waypoint symbology
        canvasCrs = QgsCoordinateReferenceSystem(4326)
        waypointPin = KadasPinItem(canvasCrs)
        waypointPin.setPosition(KadasItemPos(waypoint.x(), waypoint.y()))
        waypointPin.setup(
            ":/kadas/icons/waypoint",
            waypointPin.anchorX(),
            waypointPin.anchorX(),
            32,
            32,
        )
        self.waypointPins.append(waypointPin)
        KadasMapCanvasItemManager.addItem(waypointPin)

    def clearPins(self):
        """Remove all pins from the map
        Not removing the point stored.
        """
        # remove waypoint pins
        for waypointPin in self.waypointPins:
            KadasMapCanvasItemManager.removeItem(waypointPin)
        super().clearPins()

    def addPins(self):
        """Add pins for all stored points."""
        for waypoint in self.waypoints:
            self.addWaypointPin(waypoint)
        super().addPins()
