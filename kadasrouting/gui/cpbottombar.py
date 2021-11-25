import os
import logging
import json

from PyQt5 import uic
from PyQt5.QtGui import QColor

from qgis.core import QgsWkbTypes, QgsProject, QgsVectorLayer
from qgis.utils import iface
from qgis.gui import QgsMapToolPan
from kadasrouting.gui.valhallaroutebottombar import ValhallaRouteBottomBar
from kadasrouting.gui.patrolwarning import PatrolWarning
from kadasrouting.gui.drawpolygonmaptool import DrawPolygonMapTool
from kadasrouting.utilities import pushWarning, transformToWGS
from kadasrouting.core.canvaslayersaver import CanvasLayerSaver


# Royal Blue
PATROL_AREA_COLOR = QColor(65, 105, 225)
PATROL_AREA_STYLE = os.path.join(
    os.path.dirname(os.path.dirname(__file__)), "resources", "patrol_area.qml"
)
WIDGET, BASE = uic.loadUiType(os.path.join(os.path.dirname(__file__), "cpbottombar.ui"))

LOG = logging.getLogger(__name__)


class CPBottomBar(ValhallaRouteBottomBar, WIDGET):
    def __init__(self, canvas, action, plugin):
        self.default_layer_name = "Patrol"
        self.patrolArea = None
        self.patrolFootprint = self.createFootprintArea(color=PATROL_AREA_COLOR)
        super().__init__(canvas, action, plugin)

        self.btnPatrolAreaClear.clicked.connect(self.clearPatrol)
        self.btnPatrolAreaSave.clicked.connect(self.savePatrolAreaLayer)
        self.btnPatrolAreaCanvas.toggled.connect(
            self.setPatrolPolygonDrawingMapTool
        )  # todo: use new instance of drawing tool

        self.radioPatrolAreaPolygon.toggled.connect(self._radioButtonsPatrolChanged)
        self.radioPatrolAreaLayer.toggled.connect(self._radioButtonsPatrolChanged)
        self.radioPatrolAreaPolygon.setChecked(True)

    def populatePatrolLayerSelector(self):
        self.comboPatrolAreaLayers.clear()
        for layer in QgsProject.instance().mapLayers().values():
            if (
                isinstance(layer, QgsVectorLayer)
                and layer.geometryType() == QgsWkbTypes.PolygonGeometry
            ):
                self.comboPatrolAreaLayers.addItem(layer.name(), layer)

    def setPatrolPolygonDrawingMapTool(self, checked):
        if checked:
            self.prevMapTool = iface.mapCanvas().mapTool()
            self.mapToolDrawPolygon = DrawPolygonMapTool(iface.mapCanvas())
            self.mapToolDrawPolygon.polygonSelected.connect(
                self.setPatrolAreaFromPolygon
            )
            iface.mapCanvas().setMapTool(self.mapToolDrawPolygon)
        else:
            try:
                iface.mapCanvas().setMapTool(self.prevMapTool)
            except Exception as e:
                LOG.error(e)
                iface.mapCanvas().setMapTool(QgsMapToolPan(iface.mapCanvas()))

    def clearPatrolArea(self):
        self.patrolArea = None
        self.patrolFootprint.reset(QgsWkbTypes.PolygonGeometry)

    def setPatrolAreaFromPolygon(self, polygon):
        self.patrolArea = polygon
        self.patrolFootprint.setToGeometry(polygon)

    def _radioButtonsPatrolChanged(self):
        self.populatePatrolLayerSelector()
        self.comboPatrolAreaLayers.setEnabled(self.radioPatrolAreaLayer.isChecked())
        self.btnPatrolAreaCanvas.setEnabled(self.radioPatrolAreaPolygon.isChecked())
        self.btnPatrolAreaClear.setEnabled(self.radioPatrolAreaPolygon.isChecked())
        self.btnPatrolAreaSave.setEnabled(self.radioPatrolAreaPolygon.isChecked())
        if self.radioPatrolAreaPolygon.isChecked():
            if self.patrolArea is not None:
                self.patrolFootprint.setToGeometry(self.patrolArea)
        else:
            iface.mapCanvas().setMapTool(QgsMapToolPan(iface.mapCanvas()))
            self.patrolFootprint.reset(QgsWkbTypes.PolygonGeometry)

    def clearPatrol(self):
        self.patrolArea = None
        self.patrolFootprint.reset(QgsWkbTypes.PolygonGeometry)

    def savePatrolAreaLayer(self):
        name = "patrol_area"
        CanvasLayerSaver(
            name,
            [self.patrolArea],
            crs=self.canvas.mapSettings().destinationCrs(),
            color=PATROL_AREA_COLOR,
            style=PATROL_AREA_STYLE,
        )
        iface.mapCanvas().setMapTool(QgsMapToolPan(iface.mapCanvas()))

    def prepareValhalla(self):
        (
            layer,
            points,
            profile,
            allAreasToAvoidWGS,
            costingOptions,
        ) = super().prepareValhalla()
        if self.radioPatrolAreaPolygon.isChecked():
            # Currently only single polygon is accepted
            patrolArea = self.patrolArea
            canvasCrs = self.canvas.mapSettings().destinationCrs()
            transformer = transformToWGS(canvasCrs)
            if patrolArea is None:
                # if the custom polygon button is checked, but no polygon has been drawn
                pushWarning(
                    self.tr("Custom polygon button is checked, but no polygon is drawn")
                )
                return
        elif self.radioPatrolAreaLayer.isChecked():
            patrolLayer = self.comboPatrolAreaLayers.currentData()
            if patrolLayer is not None:
                layerCrs = patrolLayer.crs()
                transformer = transformToWGS(layerCrs)
                patrolFeatures = [f for f in patrolLayer.getFeatures()]
                if len(patrolFeatures) != 1:
                    pushWarning(
                        self.tr(
                            "The polygon layer for Patrol must contain exactly only one polygon."
                        )
                    )
                    return
                else:
                    patrolArea = patrolFeatures[0].geometry()
            else:
                # If polygon layer button is checked, but no layer polygon is selected
                pushWarning(
                    self.tr(
                        "Polygon layer button is checked for Patrol, but no layer polygon is selected"
                    )
                )
                return
        else:
            # No areas to avoid
            patrolArea = None
            patrolAreaWGS = None

        # transform to WGS84 (Valhalla's requirement)
        if patrolArea:
            patrolAreaJson = json.loads(patrolArea.asJson())
            patrolAreaWGS = []
            polygon = patrolAreaJson["coordinates"][0]
            for point in polygon:
                pointWGS = transformer.transform(point[0], point[1])
                patrolAreaWGS.append([pointWGS.x(), pointWGS.y()])
        return layer, points, profile, allAreasToAvoidWGS, costingOptions, patrolAreaWGS

    def calculate(self):
        try:
            (
                layer,
                points,
                profile,
                allAreasToAvoidWGS,
                costingOptions,
                allPatrolAreaWGS,
            ) = self.prepareValhalla()
        except TypeError:
            pushWarning(self.tr("Could not compute patrol: no polygon selected"))
            return
        try:
            layer.updateRoute(
                points, profile, allAreasToAvoidWGS, costingOptions, allPatrolAreaWGS
            )
            self.btnNavigate.setEnabled(True)
        except Exception as e:
            if "Failed to find a route between two locations for Chinese Postman route" in str(e):
                self.show_chinese_postman_warning(str(e))
            else:
                pushWarning(str(e))
            LOG.error(e, exc_info=True)
            LOG.error("Could not compute route")

    def show_chinese_postman_warning(self, error_message):
        # Add location parser here
        # set the text somehow
        dialog = PatrolWarning(self)
        dialog.exec_()

    def actionToggled(self, toggled):
        super().actionToggled(toggled)
        if toggled:
            pass
        else:
            self.clearPatrol()
