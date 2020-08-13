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
from qgis.core import (
    QgsCoordinateReferenceSystem,
)

from kadasrouting.core.optimalroutelayer import OptimalRouteLayer

WIDGET, BASE = uic.loadUiType(
    os.path.join(os.path.dirname(__file__), "navigationbottombar.ui")
)

class NavigationBottomBar(KadasBottomBar, WIDGET):
    
    def __init__(self, canvas, action):
        KadasBottomBar.__init__(self, canvas, "orange")
        self.setupUi(self)
        self.setStyleSheet("QFrame { background-color: orange; }")
        self.action = action

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
        self.btnStartStop.setEnabled(isinstance(layer, OptimalRouteLayer))

    def startStopClicked(self):
        pass

    def actionToggled(self):
        pass

    def selectedLayerChanged(self):
        pass

    def startStopClicked(self):
        pass


