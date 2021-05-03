import os
import logging
import json

from PyQt5 import uic
from PyQt5.QtCore import pyqtSignal, Qt
from PyQt5.QtGui import QIcon, QColor
from PyQt5.QtWidgets import QDesktopWidget

from kadas.kadasgui import (
    KadasBottomBar,
    KadasPinItem,
    KadasItemPos,
    KadasMapCanvasItemManager,
    KadasLayerSelectionWidget,
)
from kadasrouting.gui.locationinputwidget import (
    LocationInputWidget,
    WrongLocationException,
)
from kadasrouting.core import vehicles
from kadasrouting.utilities import iconPath, pushWarning, transformToWGS

from qgis.utils import iface
from qgis.core import (
    QgsCoordinateReferenceSystem,
    QgsWkbTypes,
    QgsVectorLayer,
    QgsProject
)
from qgis.gui import (
    QgsMapTool,
    QgsRubberBand,
    QgsMapToolPan
)

from kadasrouting.core.optimalroutelayer import OptimalRouteLayer
from kadasrouting.gui.valhallaroutebottombar import ValhallaRouteBottomBar
from kadasrouting.gui.drawpolygonmaptool import DrawPolygonMapTool

AVOID_AREA_COLOR = QColor(255, 0, 0)

WIDGET, BASE = uic.loadUiType(os.path.join(os.path.dirname(__file__), "optimalroutebottombar.ui"))

LOG = logging.getLogger(__name__)

class OptimalRouteBottomBar(ValhallaRouteBottomBar, WIDGET):
    def __init__(self, canvas, action, plugin):
        self.default_layer_name = 'Route'
        super().__init__(canvas, action, plugin)
        self.btnAddWaypoints.setIcon(QIcon(":/kadas/icons/add"))
        self.btnAddWaypoints.setToolTip(self.tr("Add waypoint"))
        self.waypointsSearchBox = LocationInputWidget(canvas, locationSymbolPath=iconPath("pin_bluegray.svg"))
        self.groupBox.layout().addWidget(self.waypointsSearchBox, 0, 0)
        self.btnAddWaypoints.clicked.connect(self.addWaypoints)
        self.btnAreasToAvoidFromCanvas.toggled.connect(
            self.setPolygonDrawingMapTool)
        self.areasToAvoidFootprint = QgsRubberBand(iface.mapCanvas(), QgsWkbTypes.PolygonGeometry)

    def clearPoints(self):
        super().clearPoints()
        self.waypointsSearchBox.clearSearchBox()
        self.lineEditWaypoints.clear()
        for waypointPin in self.waypointPins:
            KadasMapCanvasItemManager.removeItem(waypointPin)
        
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
                self.lineEditWaypoints.text() + ";"
                + self.waypointsSearchBox.text()
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
        self.lineEditWaypoints.setText(';'.join(reversed(self.lineEditWaypoints.text().split(';'))))

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
        super().clearPins()
        # remove waypoint pins
        for waypointPin in self.waypointPins:
            KadasMapCanvasItemManager.removeItem(waypointPin)

    def addPins(self):
        """Add pins for all stored points."""
        super().addPins()
        for waypoint in self.waypoints:
            self.addWaypointPin(waypoint)
