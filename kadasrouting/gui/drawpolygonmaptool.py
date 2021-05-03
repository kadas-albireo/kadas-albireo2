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
