from qgis.core import (
    QgsCoordinateReferenceSystem,
    QgsCoordinateTransform,
    QgsGeometry,
    QgsProject,
    QgsWkbTypes,
)
from qgis.gui import QgsMapTool, QgsRubberBand
from qgis.PyQt.QtCore import QPoint, Qt

from .ephem_tool_widget import EphemToolWidget


class EphemTool(QgsMapTool):

    def __init__(self, iface):
        QgsMapTool.__init__(self, iface.mapCanvas())

        self.iface = iface
        self.setCursor(Qt.CursorShape.ArrowCursor)
        self.widget = None

    def clean(self):
        self.iface.mapCanvas().unsetMapTool(self)

    def activate(self):
        self.widget = EphemToolWidget(self.iface)
        self.widget.close.connect(self.close)
        self.widget.setVisible(True)
        self.pin = QgsRubberBand(self.iface.mapCanvas(), QgsWkbTypes.PointGeometry)
        self.pin.setIcon(QgsRubberBand.IconType.ICON_SVG)
        self.pin.setSvgIcon(":/kadas/icons/pin_blue", QPoint(-32, -64))
        self.pin.setEnabled(False)

        QgsMapTool.activate(self)

    def deactivate(self):
        if self.widget:
            self.widget.cleanup()
            self.widget = None

        self.pin.reset()
        del self.pin
        self.pin = None

        self.iface.mapCanvas().refresh()
        QgsMapTool.deactivate(self)

    def close(self):
        self.iface.mapCanvas().unsetMapTool(self)

    def canvasReleaseEvent(self, event):
        if event.button() == Qt.MouseButton.LeftButton:
            self.positionPicked(self.toMapCoordinates(event.pos()))
        elif event.button() == Qt.MouseButton.RightButton:
            self.iface.mapCanvas().unsetMapTool(self)

    def keyReleaseEvent(self, event):
        if event.key() == Qt.Key.Key_Escape:
            self.iface.mapCanvas().unsetMapTool(self)

    def positionPicked(self, pos):
        self.pin.reset()
        self.pin.setToGeometry(QgsGeometry.fromPointXY(pos))
        self.pin.setEnabled(True)
        self.pin.update()
        self.iface.mapCanvas().refresh()
        mapCrs = self.iface.mapCanvas().mapSettings().destinationCrs()
        wgsCrs = QgsCoordinateReferenceSystem("EPSG:4326")
        mrcCrs = QgsCoordinateReferenceSystem("EPSG:3857")
        wgsCrst = QgsCoordinateTransform(mapCrs, wgsCrs, QgsProject.instance())
        mrcCrst = QgsCoordinateTransform(mapCrs, mrcCrs, QgsProject.instance())
        self.widget.setPos(wgsCrst.transform(pos), mrcCrst.transform(pos))
        self.widget.recompute()
