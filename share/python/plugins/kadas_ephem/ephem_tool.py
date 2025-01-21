
from qgis.PyQt.QtCore import *
from qgis.PyQt.QtGui import *
from qgis.PyQt.QtWidgets import *
from qgis.core import *
from qgis.gui import *
from kadas.kadascore import *
from kadas.kadasgui import *
from kadas.kadasanalysis import *
from .ephem_tool_widget import EphemToolWidget

class EphemTool(QgsMapTool):

    def __init__(self, iface):
        QgsMapTool.__init__(self, iface.mapCanvas())

        self.iface = iface
        self.setCursor(Qt.ArrowCursor)
        self.widget = None

    def clean(self):
        self.iface.mapCanvas().unsetMapTool(self)

    def activate(self):
        self.widget = EphemToolWidget(self.iface)
        self.widget.close.connect(self.close)
        self.widget.setVisible(True)
        self.pin = KadasSymbolItem(self.iface.mapCanvas().mapSettings().destinationCrs())
        self.pin.setup( ":/kadas/icons/pin_blue", 0.5, 1.0 );
        self.pin.setVisible(False)
        KadasMapCanvasItemManager.addItem(self.pin)

        QgsMapTool.activate(self)

    def deactivate(self):
        if self.widget:
            self.widget.cleanup()
            self.widget = None
        KadasMapCanvasItemManager.removeItem(self.pin)
        self.pin = None
        QgsMapTool.deactivate(self)

    def close(self):
        self.iface.mapCanvas().unsetMapTool(self)

    def canvasReleaseEvent(self, event):
        if event.button() == Qt.LeftButton:
            self.positionPicked(self.toMapCoordinates(event.pos()))
        elif event.button() == Qt.RightButton:
            self.iface.mapCanvas().unsetMapTool(self)

    def keyReleaseEvent(self, event):
        if event.key() == Qt.Key_Escape:
            self.iface.mapCanvas().unsetMapTool( self )

    def positionPicked(self, pos):
        self.pin.setPosition(KadasItemPos.fromPoint(pos))
        self.pin.setVisible(True)
        mapCrs = self.iface.mapCanvas().mapSettings().destinationCrs()
        wgsCrs = QgsCoordinateReferenceSystem("EPSG:4326")
        mrcCrs = QgsCoordinateReferenceSystem("EPSG:3857")
        wgsCrst = QgsCoordinateTransform(mapCrs, wgsCrs, QgsProject.instance())
        mrcCrst = QgsCoordinateTransform(mapCrs, mrcCrs, QgsProject.instance())
        self.widget.setPos(wgsCrst.transform(pos), mrcCrst.transform(pos))
        self.widget.recompute()
