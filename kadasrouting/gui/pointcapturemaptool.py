# -*- coding: utf-8 -*-

from PyQt5.QtCore import Qt, pyqtSignal

from qgis.gui import QgsMapToolEmitPoint


class PointCaptureMapTool(QgsMapToolEmitPoint):
    complete = pyqtSignal()

    def __init__(self, canvas):
        QgsMapToolEmitPoint.__init__(self, canvas)

        self.canvas = canvas
        self.cursor = Qt.CrossCursor

    def activate(self):
        self.canvas.setCursor(self.cursor)

    def canvasReleaseEvent(self, event):
        self.complete.emit()