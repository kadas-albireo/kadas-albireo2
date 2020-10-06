import os
import logging

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
from kadasrouting.utilities import iconPath, pushWarning

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

AVOID_AREA_COLOR = QColor(255, 0, 0)

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
        self.areasToAvoid = None

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

        self.btnPointsClear.clicked.connect(self.clearPoints)
        self.btnReverse.clicked.connect(self.reverse)
        self.btnAddWaypoints.clicked.connect(self.addWaypoints)
        self.btnNavigate.clicked.connect(self.navigate)
        self.btnAreasToAvoidClear.clicked.connect(self.clearAreasToAvoid)
        self.btnAreasToAvoidFromCanvas.toggled.connect(self.setPolygonDrawingMapTool)

        iface.mapCanvas().mapToolSet.connect(self.mapToolSet)

        self.areasToAvoidFootprint = QgsRubberBand(iface.mapCanvas(),
                                                   QgsWkbTypes.PolygonGeometry)
        self.areasToAvoidFootprint.setStrokeColor(AVOID_AREA_COLOR)
        self.areasToAvoidFootprint.setWidth(2)

        self.populateLayerSelector()
        self.radioAreasToAvoidPolygon.toggled.connect(self._radioButtonsChanged)
        self.radioAreasToAvoidLayer.toggled.connect(self._radioButtonsChanged)
        self.radioAreasToAvoidNone.toggled.connect(self._radioButtonsChanged)
        self.radioAreasToAvoidNone.setChecked(True)

        # Handling HiDPI screen, perhaps we can make a ratio of the screen size
        size = QDesktopWidget().screenGeometry()
        if size.width() >= 3200 or size.height() >= 1800:
            self.setFixedSize(self.size() * 1.5)

    def populateLayerSelector(self):
        self.comboAreasToAvoidLayers.clear()
        for layer in QgsProject.instance().mapLayers().values():
            if isinstance(layer, QgsVectorLayer) and layer.geometryType() == QgsWkbTypes.PolygonGeometry:
                self.comboAreasToAvoidLayers.addItem(layer.name(), layer)

    def _radioButtonsChanged(self):
        self.populateLayerSelector()
        self.comboAreasToAvoidLayers.setEnabled(self.radioAreasToAvoidLayer.isChecked())
        self.btnAreasToAvoidFromCanvas.setEnabled(self.radioAreasToAvoidPolygon.isChecked())
        self.btnAreasToAvoidClear.setEnabled(self.radioAreasToAvoidPolygon.isChecked())
        if self.radioAreasToAvoidPolygon.isChecked():
            if self.areasToAvoid is not None:
                self.areasToAvoidFootprint.setToGeometry(self.areasToAvoid)
        else:
            self.areasToAvoidFootprint.reset(QgsWkbTypes.PolygonGeometry)

    def setPolygonDrawingMapTool(self, checked):
        if checked:
            self.prevMapTool = iface.mapCanvas().mapTool()
            self.mapToolDrawPolygon = DrawPolygonMapTool(iface.mapCanvas())
            self.mapToolDrawPolygon.polygonSelected.connect(self.setAreasToAvoidFromPolygon)
            iface.mapCanvas().setMapTool(self.mapToolDrawPolygon)
        else:
            try:
                iface.mapCanvas().setMapTool(self.prevMapTool)
            except Exception as e:
                logging.error(e)
                iface.mapCanvas().setMapTool(QgsMapToolPan(iface.mapCanvas()))

    def mapToolSet(self, new, old):
        if not isinstance(new, DrawPolygonMapTool):
            self.btnAreasToAvoidFromCanvas.blockSignals(True)
            self.btnAreasToAvoidFromCanvas.setChecked(False)
            self.btnAreasToAvoidFromCanvas.blockSignals(False)

    def clearAreasToAvoid(self):
        self.areasToAvoid = None
        self.areasToAvoidFootprint.reset(QgsWkbTypes.PolygonGeometry)

    def setAreasToAvoidFromPolygon(self, polygon):
        self.areasToAvoid = polygon
        self.areasToAvoidFootprint.setToGeometry(polygon)

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
            points = [self.originSearchBox.point]
            points.extend(self.waypoints)
            points.append(self.destinationSearchBox.point)
        except WrongLocationException as e:
            pushWarning(self.tr("Invalid location:") + str(e))
            return

        if None in points:
            pushWarning(self.tr("Both origin and destination points are required"))
            return

        shortest = self.radioButtonShortest.isChecked()

        vehicle = self.comboBoxVehicles.currentIndex()
        profile, costingOptions = vehicles.options_for_vehicle(vehicle)

        '''
        areasToAvoid = None
        if self.radioAreasToAvoidPolygon.isChecked():
            areasToAvoid = self.areasToAvoid
        elif self.radioAreasToAvoidLayer.isChecked():
            avoidLayer = self.comboAreasToAvoidLayers.currentData()
            if avoidLayer is not None:
                geoms = [f.geometry() for f in avoidLayer.getFeatures()]
                areasToAvoid = QgsGeometry.collectGeometry(geoms)
        # TODO: use areas to avoid
        '''

        if shortest:
            costingOptions["shortest"] = True

        try:
            layer.updateRoute(points, profile, costingOptions)
            self.btnNavigate.setEnabled(True)
        except Exception as e:
            raise
            logging.error(e, exc_info=True)
            # TODO more fine-grained error control
            pushWarning(self.tr("Could not compute route"))
            logging.error("Could not compute route")

    def clearPoints(self):
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
            self.clearAreasToAvoid()
            self.setPolygonDrawingMapTool(False)


RB_STROKE = QColor(204, 235, 239, 255)
RB_FILL = QColor(204, 235, 239, 100)


class DrawPolygonMapTool(QgsMapTool):

    polygonSelected = pyqtSignal(object)

    def __init__(self, canvas):
        QgsMapTool.__init__(self, canvas)

        self.canvas = canvas
        self.extent = None
        self.rubberBand = QgsRubberBand(
            self.canvas, QgsWkbTypes.PolygonGeometry)
        self.rubberBand.setFillColor(RB_FILL)
        self.rubberBand.setStrokeColor(RB_STROKE)
        self.rubberBand.setWidth(1)
        self.vertex_count = 1  # two points are dropped initially

    def canvasReleaseEvent(self, event):
        if event.button() == Qt.RightButton:
            if self.rubberBand is None:
                return
            # TODO: validate geom before firing signal
            self.extent.removeDuplicateNodes()
            self.polygonSelected.emit(self.extent)
            self.rubberBand.reset(QgsWkbTypes.PolygonGeometry)
            del self.rubberBand
            self.rubberBand = None
            self.vertex_count = 1  # two points are dropped initially
            return
        elif event.button() == Qt.LeftButton:
            if self.rubberBand is None:
                self.rubberBand = QgsRubberBand(
                    self.canvas, QgsWkbTypes.PolygonGeometry)
                self.rubberBand.setFillColor(RB_FILL)
                self.rubberBand.setStrokeColor(RB_STROKE)
                self.rubberBand.setWidth(1)
            self.rubberBand.addPoint(event.mapPoint())
            self.extent = self.rubberBand.asGeometry()
            self.vertex_count += 1

    def canvasMoveEvent(self, event):
        if self.rubberBand is None:
            pass
        elif not self.rubberBand.numberOfVertices():
            pass
        elif self.rubberBand.numberOfVertices() == self.vertex_count:
            if self.vertex_count == 2:
                mouse_vertex = self.rubberBand.numberOfVertices() - 1
                self.rubberBand.movePoint(mouse_vertex, event.mapPoint())
            else:
                self.rubberBand.addPoint(event.mapPoint())
        else:
            mouse_vertex = self.rubberBand.numberOfVertices() - 1
            self.rubberBand.movePoint(mouse_vertex, event.mapPoint())

    def deactivate(self):
        QgsMapTool.deactivate(self)
        if self.rubberBand is not None:
            self.rubberBand.reset(QgsWkbTypes.PolygonGeometry)
        self.deactivated.emit()
