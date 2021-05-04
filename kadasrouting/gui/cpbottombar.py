import os
import logging

from PyQt5 import uic
from PyQt5.QtCore import pyqtSignal, Qt
from PyQt5.QtGui import QColor

from qgis.core import QgsWkbTypes, QgsProject, QgsVectorLayer
from qgis.utils import iface

from qgis.gui import QgsMapTool, QgsRubberBand
from kadasrouting.gui.valhallaroutebottombar import ValhallaRouteBottomBar
from kadasrouting.gui.drawpolygonmaptool import DrawPolygonMapTool

# Royal Blue
PATROL_AREA_COLOR = QColor(65, 105, 225)

WIDGET, BASE = uic.loadUiType(os.path.join(os.path.dirname(__file__), "cpbottombar.ui"))

LOG = logging.getLogger(__name__)


# TODO: link the following widgets:
# btnPatrolAreaClear QPushButton -> clear drawing of patrol area
# btnPatrolAreaCanvas QPushButton -> activate drawing tool
# comboPatrolAreaLayers QComboBox -> select polygon layer for patrol
# radioPatrolAreaPolygon QRadioButton -> activate btnPatrolAreaCanvas and btnPatrolAreaClear
# radioPatrolAreaLayer QRadioButton -> activate comboPatrolAreaLayers


class CPBottomBar(ValhallaRouteBottomBar, WIDGET):
    def __init__(self, canvas, action, plugin):
        self.default_layer_name = "Patrol"
        self.patrolArea = None
        self.patrolFootprint = self.createFootprintArea(color = PATROL_AREA_COLOR)
        super().__init__(canvas, action, plugin)
        
        self.btnPatrolAreaClear.clicked.connect(self.clearPatrol)
        self.btnPatrolAreaCanvas.toggled.connect(self.setPatrolPolygonDrawingMapTool) # todo: use new instance of drawing tool
        
        self.radioPatrolAreaPolygon.toggled.connect(self._radioButtonsChanged)
        self.radioPatrolAreaLayer.toggled.connect(self._radioButtonsChanged)
    
    def populateLayerSelector(self):
        super().populateLayerSelector()
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

    def _radioButtonsChanged(self):
        super()._radioButtonsChanged()
        
        self.comboPatrolAreaLayers.setEnabled(self.radioPatrolAreaLayer.isChecked())
        self.btnPatrolAreaCanvas.setEnabled(
            self.radioPatrolAreaPolygon.isChecked()
        )
        self.btnPatrolAreaClear.setEnabled(self.radioPatrolAreaPolygon.isChecked())
        if self.radioPatrolAreaPolygon.isChecked():
            if self.patrolArea is not None:
                self.patrolFootprint.setToGeometry(self.patrolArea)
        else:
            self.patrolFootprint.reset(QgsWkbTypes.PolygonGeometry)

    def clearPatrol(self):
        self.patrol = None
        self.patrolFootprint.reset(QgsWkbTypes.PolygonGeometry)

