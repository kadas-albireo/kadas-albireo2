import os
import logging

from PyQt5 import uic
from PyQt5.QtGui import QIcon

from kadas.kadasgui import (
    KadasBottomBar,
    KadasPinItem,
    KadasItemPos,
    KadasMapCanvasItemManager,
    KadasLayerSelectionWidget,
)

from qgis.utils import iface 

from kadasrouting.utilities import pushWarning
from kadasrouting.gui.pointcapturemaptool import PointCaptureMapTool
from kadasrouting.core.optimalroutelayer import OptimalRouteLayer

from qgis.core import (
    QgsProject,
    QgsCoordinateTransform,
    QgsCoordinateReferenceSystem
)

WIDGET, BASE = uic.loadUiType(
    os.path.join(os.path.dirname(__file__), "navigationbottombar.ui")
)

class NavigationBottomBar(KadasBottomBar, WIDGET):
    
    def __init__(self, canvas, action):
        KadasBottomBar.__init__(self, canvas, "orange")
        self.setupUi(self)
        self.setStyleSheet("QFrame { background-color: orange; }")
        self.action = action

        self.navigating = False
        self.prevMapTool = None

        self.action.toggled.connect(self.actionToggled)
        self.btnClose.clicked.connect(self.action.toggle)
        self.btnClose.setIcon(QIcon(":/kadas/icons/close"))
        self.btnClose.setToolTip(self.tr("Close navigation dialog"))

        self.layerSelector = KadasLayerSelectionWidget(
            canvas,
            iface.layerTreeView(),
            lambda x: isinstance(x, OptimalRouteLayer),
            lambda: None
        )
        self.layerSelector.selectedLayerChanged.connect(self.selectedLayerChanged)
        self.layout().addWidget(self.layerSelector, 0, 0)
        self.btnStartStop.clicked.connect(self.startStopClicked)
        layer = self.layerSelector.getSelectedLayer()        
        self.updateWidgets()

    def actionToggled(self):
        if self.navigating:
            self.navigating = False
            self.updateNavigation()

    def selectedLayerChanged(self, layer):
        self.updateNavigation()

    def navigateForLayer(self, layer):
        self.layerSelector.setSelectedLayer(layer)
        self.navigating = True
        self.updateNavigation()

    ########## Temporary functionality to test navigation logic ######

    def showNavigationInfo(self, point, button):
        outCrs = QgsCoordinateReferenceSystem(4326)
        canvasCrs = iface.mapCanvas().mapSettings().destinationCrs()
        transform = QgsCoordinateTransform(canvasCrs, outCrs, QgsProject.instance())
        wgspoint = transform.transform(point)
        layer = self.layerSelector.getSelectedLayer()
        message = layer.maneuverForPoint(wgspoint)
        self.textBrowser.setPlainText(message)
        
    def startStopClicked(self):
        self.navigating = not self.navigating
        self.updateNavigation()

    def updateNavigation(self):
        selectedLayer = self.layerSelector.getSelectedLayer()
        self.navigating = self.navigating and isinstance(selectedLayer, OptimalRouteLayer)
        if not self.navigating:
            if self.prevMapTool is not None:
                iface.mapCanvas().setMapTool(self.prevMapTool)
        else:
            self.mapTool = PointCaptureMapTool(iface.mapCanvas())
            self.mapTool.canvasClicked.connect(self.showNavigationInfo)
            self.prevMapTool = iface.mapCanvas().mapTool()
            iface.mapCanvas().setMapTool(self.mapTool)
        self.updateWidgets()

    def updateWidgets(self):
        selectedLayer = self.layerSelector.getSelectedLayer()
        self.layerSelector.setEnabled(not self.navigating)
        self.btnStartStop.setEnabled(isinstance(selectedLayer, OptimalRouteLayer))
        self.btnStartStop.setText("Stop" if self.navigating else "Start")
        if not self.navigation:
            self.textBrowser.setPlainText("Select a route and click on Start to navigate")

