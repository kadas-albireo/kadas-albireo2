import os
import logging

from PyQt5 import uic
from PyQt5.QtCore import pyqtSignal, Qt
from PyQt5.QtGui import QColor

from qgis.core import QgsWkbTypes, QgsProject, QgsVectorLayer
from qgis.utils import iface

from qgis.gui import QgsMapTool, QgsRubberBand
from kadasrouting.gui.valhallaroutebottombar import ValhallaRouteBottomBar


PATROL_AREA_COLOR = QColor(0, 0, 255)

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
        super().__init__(canvas, action, plugin)
        self.patrol = None
        self.patrolFootprint = QgsRubberBand(
            iface.mapCanvas(), QgsWkbTypes.PolygonGeometry
        )
        self.patrolFootprint.setStrokeColor(PATROL_AREA_COLOR)
        self.patrolFootprint.setWidth(2)
        
        self.btnPatrolAreaClear.clicked.connect(self.clearPatrol)
        self.btnPatrolAreaCanvas.toggled.connect(self.setPolygonDrawingMapTool) # todo: use new instance of drawing tool
        
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
        
    def _radioButtonsChanged(self):
        print(self.patrolFootprint)
        super()._radioButtonsChanged()
        
        self.comboPatrolAreaLayers.setEnabled(self.radioPatrolAreaLayer.isChecked())
        self.btnPatrolAreaCanvas.setEnabled(
            self.radioPatrolAreaPolygon.isChecked()
        )
        self.btnPatrolAreaClear.setEnabled(self.radioPatrolAreaPolygon.isChecked())
        if self.radioPatrolAreaPolygon.isChecked():
            if self.patrol is not None:
                self.patrolFootprint.setToGeometry(self.patrol)
        else:
            self.patrolFootprint.reset(QgsWkbTypes.PolygonGeometry)

    def clearPatrol(self):
        self.patrol = None
