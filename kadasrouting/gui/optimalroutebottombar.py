import os
import logging

from PyQt5 import uic
from PyQt5.QtGui import QIcon
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
from kadasrouting.utilities import iconPath, pushWarning

from qgis.utils import iface
from qgis.core import (
    QgsCoordinateReferenceSystem,
)

from kadasrouting.core.optimalroutelayer import OptimalRouteLayer

WIDGET, BASE = uic.loadUiType(
    os.path.join(os.path.dirname(__file__), "optimalroutebottombar.ui")
)


class OptimalRouteBottomBar(KadasBottomBar, WIDGET):
    def __init__(self, canvas, action, plugin):
        KadasBottomBar.__init__(self, canvas, "orange")
        self.setupUi(self)
        self.setStyleSheet("QFrame { background-color: orange; }")
        self.action = action
        self.plugin = plugin
        self.canvas = canvas
        self.waypoints = []
        self.waypointPins = []

        self.btnAddWaypoints.setIcon(QIcon(":/kadas/icons/add"))
        self.btnClose.setIcon(QIcon(":/kadas/icons/close"))
        self.btnAddWaypoints.setToolTip(self.tr("Add waypoint"))
        self.btnClose.setToolTip(self.tr("Close routing dialog"))

        self.action.toggled.connect(self.actionToggled)
        self.btnClose.clicked.connect(self.action.toggle)

        self.btnCalculate.clicked.connect(self.calculate)

        self.layerSelector = KadasLayerSelectionWidget(
            canvas,
            iface.layerTreeView(),
            lambda x: isinstance(x, OptimalRouteLayer),
            self.createLayer,
        )
        self.layerSelector.createLayerIfEmpty(self.tr("Route"))
        self.layerSelector.selectedLayerChanged.connect(self.selectedLayerChanged)
        self.layout().addWidget(self.layerSelector, 0, 0, 1, 2)
        layer = self.layerSelector.getSelectedLayer()
        self.btnNavigate.setEnabled(layer is not None and layer.hasRoute())

        self.originSearchBox = LocationInputWidget(
            canvas, locationSymbolPath=iconPath("pin_origin.svg")
        )
        self.layout().addWidget(self.originSearchBox, 2, 1)

        self.destinationSearchBox = LocationInputWidget(
            canvas, locationSymbolPath=iconPath("pin_destination.svg")
        )
        self.layout().addWidget(self.destinationSearchBox, 3, 1)

        self.waypointsSearchBox = LocationInputWidget(
            canvas, locationSymbolPath=iconPath("pin_bluegray.svg")
        )
        self.groupBox.layout().addWidget(self.waypointsSearchBox, 0, 0)

        self.comboBoxVehicles.addItems(vehicles.vehicle_names())

        self.pushButtonClear.clicked.connect(self.clear)
        self.pushButtonReverse.clicked.connect(self.reverse)
        self.btnAddWaypoints.clicked.connect(self.addWaypoints)
        self.btnNavigate.clicked.connect(self.navigate)

        # Handling HiDPI screen, perhaps we can make a ratio of the screen size
        size = QDesktopWidget().screenGeometry()
        if size.width() >= 3200 or size.height() >= 1800:
            self.setFixedSize(self.size() * 1.5)

    def createLayer(self, name):
        layer = OptimalRouteLayer(name)
        return layer

    def selectedLayerChanged(self, layer):    
        self.btnNavigate.setEnabled(layer is not None and layer.hasRoute())
    
    def calculate(self):
        layer = self.layerSelector.getSelectedLayer()
        if layer is None:
            pushWarning(self.tr("Please, select a valid destination layer"))
            return
        try:
            points = [self.originSearchBox.valueAsPoint()]
            points.extend(self.waypoints)
            points.append(self.destinationSearchBox.valueAsPoint())
        except WrongLocationException as e:
            pushWarning(self.tr("Invalid location:") + str(e))
            return

        shortest = self.radioButtonShortest.isChecked()

        vehicle = self.comboBoxVehicles.currentIndex()
        profile, costingOptions = vehicles.options_for_vehicle(vehicle)

        if shortest:
            if profile == "auto":
                profile = "auto_shorter"
            else:
                pushWarning("Shortest path is not compatible with the selected vehicle")
                return
        try:
            layer.updateRoute(points, profile, costingOptions)
            self.btnNavigate.setEnabled(True)
        except Exception as e:
            logging.error(e, exc_info=True)            
            # TODO more fine-grained error control
            pushWarning(self.tr("Could not compute route"))
            logging.error("Could not compute route")

    def clear(self):
        self.originSearchBox.clearSearchBox()
        self.destinationSearchBox.clearSearchBox()
        self.waypointsSearchBox.clearSearchBox()
        self.waypoints = []
        self.lineEditWaypoints.clear()
        for waypointPin in self.waypointPins:
            KadasMapCanvasItemManager.removeItem(waypointPin)
        self.waypointPins = []
        KadasMapCanvasItemManager.removeItem(self.originSearchBox.pin)
        KadasMapCanvasItemManager.removeItem(self.destinationSearchBox.pin)

    def addWaypoints(self):
        """Add way point to the list of way points"""
        if self.waypointsSearchBox.text() == "":
            return
        waypoint = self.waypointsSearchBox.valueAsPoint()
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
        """Reverse route"""
        originLocation = self.originSearchBox.text()
        self.originSearchBox.setText(self.destinationSearchBox.text())
        self.destinationSearchBox.setText(originLocation)
        self.waypoints.reverse()
        self.waypointPins.reverse()
        waypointsCoordinates = []
        for waypoint in self.waypoints:
            waypointsCoordinates.append("%f, %f" % (waypoint.x(), waypoint.y()))
        self.lineEditWaypoints.setText(";".join(waypointsCoordinates))
        self.clearPins()
        self.addPins()

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
        # remove origin pin
        self.originSearchBox.removePin()
        # remove destination poin
        self.destinationSearchBox.removePin()
        # remove waypoint pins
        for waypointPin in self.waypointPins:
            KadasMapCanvasItemManager.removeItem(waypointPin)

    def addPins(self):
        """Add pins for all stored points."""
        self.originSearchBox.addPin()
        self.destinationSearchBox.addPin()
        for waypoint in self.waypoints:
            self.addWaypointPin(waypoint)

    def navigate(self):
        self.action.toggle()
        iface.setActiveLayer(self.layerSelector.getSelectedLayer())
        self.plugin.navigationAction.toggle()       
        
    def actionToggled(self, toggled):
        if toggled:
            self.addPins()
        else:
            self.clearPins()
