import logging
import json

from PyQt5.QtGui import QIcon, QColor
from PyQt5.QtWidgets import QDesktopWidget

from kadas.kadasgui import (
    KadasBottomBar,
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
    QgsWkbTypes,
    QgsVectorLayer,
    QgsProject,
)
from qgis.gui import QgsRubberBand, QgsMapToolPan

from kadasrouting.core.optimalroutelayer import OptimalRouteLayer
from kadasrouting.gui.drawpolygonmaptool import DrawPolygonMapTool


AVOID_AREA_COLOR = QColor(255, 0, 0)
LOG = logging.getLogger(__name__)


class ValhallaRouteBottomBar(KadasBottomBar):
    """
    This is a meta class and should not be instanciated by itself, as it is has no
    UI file associated with it. Use this only as Parent class.
    """

    def __init__(self, canvas, action, plugin):
        KadasBottomBar.__init__(self, canvas, "orange")
        self.setupUi(self)
        self.setStyleSheet("QFrame { background-color: orange; }")
        self.action = action
        self.plugin = plugin
        self.canvas = canvas
        self.patrolArea = None
        self.waypoints = []
        self.waypointPins = []
        self.areasToAvoid = None

        self.btnClose.setIcon(QIcon(":/kadas/icons/close"))
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
        self.layerSelector.createLayerIfEmpty(self.tr(self.default_layer_name))
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

        self.comboBoxVehicles.addItems(vehicles.vehicle_names())

        self.btnPointsClear.clicked.connect(self.clearPoints)
        self.btnReverse.clicked.connect(self.reverse)
        self.btnNavigate.clicked.connect(self.navigate)
        self.btnAreasToAvoidClear.clicked.connect(self.clearAreasToAvoid)
        self.btnAreasToAvoidFromCanvas.toggled.connect(self.setPolygonDrawingMapTool)

        iface.mapCanvas().mapToolSet.connect(self.mapToolSet)

        self.areasToAvoidFootprint = QgsRubberBand(
            iface.mapCanvas(), QgsWkbTypes.PolygonGeometry
        )
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
            if (
                isinstance(layer, QgsVectorLayer)
                and layer.geometryType() == QgsWkbTypes.PolygonGeometry
            ):
                self.comboAreasToAvoidLayers.addItem(layer.name(), layer)

    def _radioButtonsChanged(self):
        self.populateLayerSelector()
        self.comboAreasToAvoidLayers.setEnabled(self.radioAreasToAvoidLayer.isChecked())
        self.btnAreasToAvoidFromCanvas.setEnabled(
            self.radioAreasToAvoidPolygon.isChecked()
        )
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
            self.mapToolDrawPolygon.polygonSelected.connect(
                self.setAreasToAvoidFromPolygon
            )
            iface.mapCanvas().setMapTool(self.mapToolDrawPolygon)
        else:
            try:
                iface.mapCanvas().setMapTool(self.prevMapTool)
            except Exception as e:
                LOG.error(e)
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
            if len(self.waypoints) > 0:
                points.extend(self.waypoints)
            points.append(self.destinationSearchBox.point)
        except WrongLocationException as e:
            pushWarning(self.tr("Invalid location:") + str(e))
            return

        if None in points:
            pushWarning(self.tr("Both origin and destination points are required"))
            return
        try:
            shortest = self.radioButtonShortest.isChecked()
        except AttributeError:
            shortest = False
        vehicle = self.comboBoxVehicles.currentIndex()
        profile, costingOptions = vehicles.options_for_vehicle(vehicle)

        if self.radioAreasToAvoidPolygon.isChecked():
            # Currently only single polygon is accepted
            areasToAvoid = self.areasToAvoid
            canvasCrs = self.canvas.mapSettings().destinationCrs()
            transformer = transformToWGS(canvasCrs)
            if areasToAvoid is None:
                # if the custom polygon button is checked, but no polygon has been drawn
                pushWarning(
                    self.tr("Custom polygon button is checked, but no polygon is drawn")
                )
                return
            # make it a list to have the same data type as in the avoid layer
            areasToAvoid = [areasToAvoid]
        elif self.radioAreasToAvoidLayer.isChecked():
            avoidLayer = self.comboAreasToAvoidLayers.currentData()
            if avoidLayer is not None:
                layerCrs = avoidLayer.crs()
                transformer = transformToWGS(layerCrs)
                areasToAvoid = [f.geometry() for f in avoidLayer.getFeatures()]
            else:
                # If polygon layer button is checked, but no layer polygon is selected
                pushWarning(
                    self.tr(
                        "Polygon layer button is checked, but no layer polygon is selected"
                    )
                )
                return
        else:
            # No areas to avoid
            areasToAvoid = None
            areasToAvoidWGS = None
            allAreasToAvoidWGS = None

        if shortest:
            costingOptions["shortest"] = True

        # transform to WGS84 (Valhalla's requirement)
        allAreasToAvoidWGS = []
        if areasToAvoid:
            for areasToAvoidGeom in areasToAvoid:
                areasToAvoidJson = json.loads(areasToAvoidGeom.asJson())
                areasToAvoidWGS = []
                for i, polygon in enumerate(areasToAvoidJson["coordinates"]):
                    areasToAvoidWGS.append([])
                    for point in polygon:
                        pointWGS = transformer.transform(point[0], point[1])
                        areasToAvoidWGS[i].append([pointWGS.x(), pointWGS.y()])
                allAreasToAvoidWGS.extend(areasToAvoidWGS)
        try:
            layer.updateRoute(points, profile, allAreasToAvoidWGS, costingOptions)
            self.btnNavigate.setEnabled(True)
        except Exception as e:
            LOG.error(e, exc_info=True)
            # TODO more fine-grained error control
            pushWarning(self.tr("Could not compute route"))
            LOG.error("Could not compute route")
            raise (e)

    def clearPoints(self):
        self.originSearchBox.clearSearchBox()
        self.destinationSearchBox.clearSearchBox()
        KadasMapCanvasItemManager.removeItem(self.originSearchBox.pin)
        KadasMapCanvasItemManager.removeItem(self.destinationSearchBox.pin)

    def reverse(self):
        """Reverse route"""
        self.clearPins()

        # Reverse origin and destination
        originPointText = self.originSearchBox.text()
        originPoint = self.originSearchBox.point
        self.originSearchBox.setText(self.destinationSearchBox.text())
        self.originSearchBox.setPoint(self.destinationSearchBox.point)
        self.destinationSearchBox.setText(originPointText)
        self.destinationSearchBox.setPoint(originPoint)

        self.addPins()

    def clearPins(self):
        """Remove all pins from the map
        Not removing the point stored.
        """
        # remove origin pin
        self.originSearchBox.removePin()
        # remove destination poin
        self.destinationSearchBox.removePin()

    def addPins(self):
        """Add pins for all stored points."""
        self.originSearchBox.addPin()
        self.destinationSearchBox.addPin()

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
