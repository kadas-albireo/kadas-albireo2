import os
import logging

from PyQt5 import uic
from PyQt5.QtGui import QIcon
from PyQt5.QtWidgets import QDesktopWidget

from kadas.kadasgui import (
    KadasBottomBar, 
    KadasPinItem, 
    KadasItemPos, 
    KadasMapCanvasItemManager, 
    KadasLayerSelectionWidget
    )
from kadasrouting.gui.locationinputwidget import LocationInputWidget, WrongLocationException
from kadasrouting import vehicles
from kadasrouting.utilities import iconPath, pushMessage, pushWarning

from qgis.utils import iface
from qgis.core import (
    Qgis,
    QgsProject,
    QgsCoordinateReferenceSystem,
    )

from kadasrouting.core.shortestpathlayer import ShortestPathLayer, RoutePointMapItem

WIDGET, BASE = uic.loadUiType(os.path.join(os.path.dirname(__file__), 'tspbottombar.ui'))

class TSPBottomBar(KadasBottomBar, WIDGET):

    def __init__(self, canvas, action):
        KadasBottomBar.__init__(self, canvas, "orange")
        self.setupUi(self)
        self.setStyleSheet("QFrame { background-color: orange; }")
        self.action = action

        self.btnClose.setIcon(QIcon(":/kadas/icons/close"))
        self.btnClose.setToolTip('Close TSP dialog')

        self.action.toggled.connect(self.actionToggled)
        self.btnClose.clicked.connect(self.action.toggle)

        self.btnCalculate.clicked.connect(self.calculate)

        self.layerSelector = KadasLayerSelectionWidget(canvas, iface.layerTreeView(),
                                                        lambda x: isinstance(x, ShortestPathLayer),
                                                        self.createLayer)
        self.layerSelector.createLayerIfEmpty("TSP")
        self.layout().addWidget(self.layerSelector, 0, 0, 1, 2)

        self.originSearchBox = LocationInputWidget(canvas, locationSymbolPath=iconPath('pin_origin.svg'))
        self.layout().addWidget(self.originSearchBox, 2, 1)

        self.destinationSearchBox = LocationInputWidget(canvas, locationSymbolPath=iconPath('pin_destination.svg'))
        self.layout().addWidget(self.destinationSearchBox, 3, 1)

        # Set default
        self.lineEditBuffer.setText('500')

        self.pushButtonReset.clicked.connect(self.clear)

    def createLayer(self, name):
        layer = ShortestPathLayer(name)
        return layer

    def calculate(self):
        pushMessage('Calculating TSP.')

    def clear(self):
        self.originSearchBox.clearSearchBox()
        self.destinationSearchBox.clearSearchBox()
        KadasMapCanvasItemManager.removeItem(self.originSearchBox.pin)
        KadasMapCanvasItemManager.removeItem(self.destinationSearchBox.pin)
        self.lineEditBuffer.setText('')

    def clearPins(self):
        """Remove all pins from the map
        Not removing the point stored.
        """
        # remove origin pin
        self.originSearchBox.removePin()
        # remove destination poin
        self.destinationSearchBox.removePin()

    def addPins(self):
        """Add pins for all stored points."""
        self.originSearchBox.addPin()
        self.destinationSearchBox.addPin()

    def actionToggled(self, toggled):
        if toggled:
            self.addPins()
        else:
            self.clearPins()