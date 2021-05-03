import os
import logging

from PyQt5 import uic
from PyQt5.QtCore import pyqtSignal, Qt
from PyQt5.QtGui import QColor

from qgis.core import QgsWkbTypes

from qgis.gui import QgsMapTool, QgsRubberBand
from kadasrouting.gui.valhallaroutebottombar import ValhallaRouteBottomBar


AVOID_AREA_COLOR = QColor(255, 0, 0)

WIDGET, BASE = uic.loadUiType(os.path.join(os.path.dirname(__file__), "cpbottombar.ui"))

LOG = logging.getLogger(__name__)


# TODO: link the following widgets:
# btnPatrolAreaCanvas QPushButton -> activate drawing tool
# btnPatrolAreaClear QPushButton -> clear drawing of patrol area
# comboPatrolAreaLayers QComboBox -> select polygon layer for patrol
# radioPatrolAreaPolygon QRadioButton -> activate btnPatrolAreaCanvas and btnPatrolAreaClear
# radioPatrolAreaLayer QRadioButton -> activate comboPatrolAreaLayers


class CPBottomBar(ValhallaRouteBottomBar, WIDGET):
    def __init__(self, canvas, action, plugin):
        super().__init__(canvas, action, plugin)


# class CPBottomBar(KadasBottomBar, WIDGET):
#     def __init__(self, canvas, action, plugin):
#         KadasBottomBar.__init__(self, canvas, "orange")
#         self.setupUi(self)
#         self.setStyleSheet("QFrame { background-color: orange; }")
#         self.action = action
#         self.plugin = plugin
#         self.canvas = canvas
#         self.patrolArea = None
#         self.areasToAvoid = None

#         self.btnClose.setIcon(QIcon(":/kadas/icons/close"))
#         self.btnClose.setToolTip(self.tr("Close routing dialog"))

#         self.action.toggled.connect(self.actionToggled)
#         self.btnClose.clicked.connect(self.action.toggle)

#         self.btnCalculate.clicked.connect(self.calculate)

#         self.layerSelector = KadasLayerSelectionWidget(
#             canvas,
#             iface.layerTreeView(),
#             lambda x: isinstance(x, OptimalRouteLayer),
#             self.createLayer,
#         )
#         self.layerSelector.createLayerIfEmpty(self.tr("Patrol"))
#         self.layerSelector.selectedLayerChanged.connect(self.selectedLayerChanged)
#         self.layout().addWidget(self.layerSelector, 0, 0, 1, 2)
#         layer = self.layerSelector.getSelectedLayer()
#         self.btnNavigate.setEnabled(layer is not None and layer.hasRoute())

#         self.originSearchBox = LocationInputWidget(
#             canvas, locationSymbolPath=iconPath("pin_origin.svg")
#         )
#         self.layout().addWidget(self.originSearchBox, 2, 1)

#         self.destinationSearchBox = LocationInputWidget(
#             canvas, locationSymbolPath=iconPath("pin_destination.svg")
#         )
#         self.layout().addWidget(self.destinationSearchBox, 3, 1)

#         self.comboBoxVehicles.addItems(vehicles.vehicle_names())

#         self.btnPointsClear.clicked.connect(self.clearPoints)
#         self.btnReverse.clicked.connect(self.reverse)
#         self.btnNavigate.clicked.connect(self.navigate)
#         self.btnAreasToAvoidClear.clicked.connect(self.clearAreasToAvoid)
#         self.btnAreasToAvoidFromCanvas.toggled.connect(self.setPolygonDrawingMapTool)

#         iface.mapCanvas().mapToolSet.connect(self.mapToolSet)

#         self.areasToAvoidFootprint = QgsRubberBand(
#             iface.mapCanvas(), QgsWkbTypes.PolygonGeometry
#         )
#         self.areasToAvoidFootprint.setStrokeColor(AVOID_AREA_COLOR)
#         self.areasToAvoidFootprint.setWidth(2)

#         self.populateLayerSelector()
#         self.radioAreasToAvoidPolygon.toggled.connect(self._radioButtonsChanged)
#         self.radioAreasToAvoidLayer.toggled.connect(self._radioButtonsChanged)
#         self.radioAreasToAvoidNone.toggled.connect(self._radioButtonsChanged)
#         self.radioAreasToAvoidNone.setChecked(True)

#         # Handling HiDPI screen, perhaps we can make a ratio of the screen size
#         size = QDesktopWidget().screenGeometry()
#         if size.width() >= 3200 or size.height() >= 1800:
#             self.setFixedSize(self.size() * 1.5)

#     def populateLayerSelector(self):
#         self.comboAreasToAvoidLayers.clear()
#         for layer in QgsProject.instance().mapLayers().values():
#             if (
#                 isinstance(layer, QgsVectorLayer)
#                 and layer.geometryType() == QgsWkbTypes.PolygonGeometry
#             ):
#                 self.comboAreasToAvoidLayers.addItem(layer.name(), layer)

#     def _radioButtonsChanged(self):
#         self.populateLayerSelector()
#         self.comboAreasToAvoidLayers.setEnabled(self.radioAreasToAvoidLayer.isChecked())
#         self.btnAreasToAvoidFromCanvas.setEnabled(
#             self.radioAreasToAvoidPolygon.isChecked()
#         )
#         self.btnAreasToAvoidClear.setEnabled(self.radioAreasToAvoidPolygon.isChecked())
#         if self.radioAreasToAvoidPolygon.isChecked():
#             if self.areasToAvoid is not None:
#                 self.areasToAvoidFootprint.setToGeometry(self.areasToAvoid)
#         else:
#             self.areasToAvoidFootprint.reset(QgsWkbTypes.PolygonGeometry)

#     def setPolygonDrawingMapTool(self, checked):
#         if checked:
#             self.prevMapTool = iface.mapCanvas().mapTool()
#             self.mapToolDrawPolygon = DrawPolygonMapTool(iface.mapCanvas())
#             self.mapToolDrawPolygon.polygonSelected.connect(
#                 self.setAreasToAvoidFromPolygon
#             )
#             iface.mapCanvas().setMapTool(self.mapToolDrawPolygon)
#         else:
#             try:
#                 iface.mapCanvas().setMapTool(self.prevMapTool)
#             except Exception as e:
#                 LOG.error(e)
#                 iface.mapCanvas().setMapTool(QgsMapToolPan(iface.mapCanvas()))

#     def mapToolSet(self, new, old):
#         if not isinstance(new, DrawPolygonMapTool):
#             self.btnAreasToAvoidFromCanvas.blockSignals(True)
#             self.btnAreasToAvoidFromCanvas.setChecked(False)
#             self.btnAreasToAvoidFromCanvas.blockSignals(False)

#     def clearAreasToAvoid(self):
#         self.areasToAvoid = None
#         self.areasToAvoidFootprint.reset(QgsWkbTypes.PolygonGeometry)

#     def setAreasToAvoidFromPolygon(self, polygon):
#         self.areasToAvoid = polygon
#         self.areasToAvoidFootprint.setToGeometry(polygon)

#     def createLayer(self, name):
#         layer = OptimalRouteLayer(name)
#         return layer

#     def selectedLayerChanged(self, layer):
#         self.btnNavigate.setEnabled(layer is not None and layer.hasRoute())

#     def calculate(self):
#         layer = self.layerSelector.getSelectedLayer()
#         if layer is None:
#             pushWarning(self.tr("Please, select a valid destination layer"))
#             return
#         try:
#             points = [self.originSearchBox.point]
#             points.append(self.destinationSearchBox.point)
#         except WrongLocationException as e:
#             pushWarning(self.tr("Invalid location:") + str(e))
#             return

#         if None in points:
#             pushWarning(self.tr("Both origin and destination points are required"))
#             return

#         vehicle = self.comboBoxVehicles.currentIndex()
#         profile, costingOptions = vehicles.options_for_vehicle(vehicle)

#         if self.radioAreasToAvoidPolygon.isChecked():
#             # Currently only single polygon is accepted
#             areasToAvoid = self.areasToAvoid
#             canvasCrs = self.canvas.mapSettings().destinationCrs()
#             transformer = transformToWGS(canvasCrs)
#             if areasToAvoid is None:
#                 # if the custom polygon button is checked, but no polygon has been drawn
#                 pushWarning(
#                     self.tr("Custom polygon button is checked, but no polygon is drawn")
#                 )
#                 return
#             # make it a list to have the same data type as in the avoid layer
#             areasToAvoid = [areasToAvoid]
#         elif self.radioAreasToAvoidLayer.isChecked():
#             avoidLayer = self.comboAreasToAvoidLayers.currentData()
#             if avoidLayer is not None:
#                 layerCrs = avoidLayer.crs()
#                 transformer = transformToWGS(layerCrs)
#                 areasToAvoid = [f.geometry() for f in avoidLayer.getFeatures()]
#             else:
#                 # If polygon layer button is checked, but no layer polygon is selected
#                 pushWarning(
#                     self.tr(
#                         "Polygon layer button is checked, but no layer polygon is selected"
#                     )
#                 )
#                 return
#         else:
#             # No areas to avoid
#             areasToAvoid = None
#             areasToAvoidWGS = None
#             allAreasToAvoidWGS = None

#         # transform to WGS84 (Valhalla's requirement)
#         allAreasToAvoidWGS = []
#         if areasToAvoid:
#             for areasToAvoidGeom in areasToAvoid:
#                 areasToAvoidJson = json.loads(areasToAvoidGeom.asJson())
#                 areasToAvoidWGS = []
#                 for i, polygon in enumerate(areasToAvoidJson["coordinates"]):
#                     areasToAvoidWGS.append([])
#                     for point in polygon:
#                         pointWGS = transformer.transform(point[0], point[1])
#                         areasToAvoidWGS[i].append([pointWGS.x(), pointWGS.y()])
#                 allAreasToAvoidWGS.extend(areasToAvoidWGS)
#         try:
#             layer.updateRoute(points, profile, allAreasToAvoidWGS, costingOptions)
#             self.btnNavigate.setEnabled(True)
#         except Exception as e:
#             LOG.error(e, exc_info=True)
#             # TODO more fine-grained error control
#             pushWarning(self.tr("Could not compute route"))
#             LOG.error("Could not compute route")
#             raise (e)

#     def clearPoints(self):
#         self.originSearchBox.clearSearchBox()
#         self.destinationSearchBox.clearSearchBox()
#         KadasMapCanvasItemManager.removeItem(self.originSearchBox.pin)
#         KadasMapCanvasItemManager.removeItem(self.destinationSearchBox.pin)

#     def reverse(self):
#         """Reverse route"""
#         self.clearPins()

#         # Reverse origin and destination
#         originPointText = self.originSearchBox.text()
#         originPoint = self.originSearchBox.point
#         self.originSearchBox.setText(self.destinationSearchBox.text())
#         self.originSearchBox.setPoint(self.destinationSearchBox.point)
#         self.destinationSearchBox.setText(originPointText)
#         self.destinationSearchBox.setPoint(originPoint)

#         self.addPins()

#     def clearPins(self):
#         """Remove all pins from the map
#         Not removing the point stored.
#         """
#         # remove origin pin
#         self.originSearchBox.removePin()
#         # remove destination poin
#         self.destinationSearchBox.removePin()

#     def addPins(self):
#         """Add pins for all stored points."""
#         self.originSearchBox.addPin()
#         self.destinationSearchBox.addPin()

#     def navigate(self):
#         self.action.toggle()
#         iface.setActiveLayer(self.layerSelector.getSelectedLayer())
#         self.plugin.navigationAction.toggle()

#     def actionToggled(self, toggled):
#         if toggled:
#             self.addPins()
#         else:
#             self.clearPins()
#             self.clearAreasToAvoid()
#             self.setPolygonDrawingMapTool(False)


RB_STROKE = QColor(204, 235, 239, 255)
RB_FILL = QColor(204, 235, 239, 100)


class DrawPolygonMapTool(QgsMapTool):

    polygonSelected = pyqtSignal(object)

    def __init__(self, canvas):
        QgsMapTool.__init__(self, canvas)

        self.canvas = canvas
        self.extent = None
        self.rubberBand = QgsRubberBand(self.canvas, QgsWkbTypes.PolygonGeometry)
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
                    self.canvas, QgsWkbTypes.PolygonGeometry
                )
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
