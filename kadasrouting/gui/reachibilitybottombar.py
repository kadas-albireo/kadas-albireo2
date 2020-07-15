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

from kadasrouting.core.isochroneslayer import IsochronesLayer

WIDGET, BASE = uic.loadUiType(os.path.join(os.path.dirname(__file__), 'reachibilitybottombar.ui'))

class ReachibilityBottomBar(KadasBottomBar, WIDGET):

    def __init__(self, canvas, action):
        KadasBottomBar.__init__(self, canvas, "orange")
        self.setupUi(self)
        self.setStyleSheet("QFrame { background-color: orange; }")
        self.action = action
        self.canvas = canvas

        self.btnClose.setIcon(QIcon(":/kadas/icons/close"))
        self.btnClose.setToolTip('Close reachibility dialog')

        self.action.toggled.connect(self.actionToggled)
        self.btnClose.clicked.connect(self.action.toggle)

        self.btnCalculate.clicked.connect(self.calculate)

        self.layerSelector = KadasLayerSelectionWidget(canvas, iface.layerTreeView(),
                                                        lambda x: isinstance(x, IsochronesLayer),
                                                        self.createLayer)
        self.layerSelector.createLayerIfEmpty("Isochrones")
        self.layout().addWidget(self.layerSelector, 0, 0, 1, 2)

        self.originSearchBox = LocationInputWidget(canvas, locationSymbolPath=iconPath('pin_origin.svg'))
        self.layout().addWidget(self.originSearchBox, 3, 1)

        self.comboBoxVehicles.addItems(vehicles.vehicles)

        self.comboBoxReachibiliyMode.addItems(
            ['Isochrone', 'Isodistance']
        )

        self.lineEditIntervals.textChanged.connect(self.intervalChanges)
        self.lineEditIntervals.setToolTip(
            'Set interval in minutes, separated by ";" symbol')

        # Handling HiDPI screen, perhaps we can make a ratio of the screen size
        # size = QDesktopWidget().screenGeometry()
        # if size.width() >= 3200 or size.height() >= 1800:
        #     self.setFixedSize(self.size() / 1.5)

    def createLayer(self, name):
        layer = IsochronesLayer(name)
        return layer
        

    def calculate(self):
        clear = self.checkBoxRemovePrevious.isChecked()
        layer = self.layerSelector.getSelectedLayer()
        if layer is None:
            pushWarning("Please, select a valid destination layer")
            return
        try:
            point = self.originSearchBox.valueAsPoint()
            intervals = self.getInterval()
        except WrongLocationException as e:
            pushWarning("Invalid location %s" % str(e))
            return
        try:
            layer.updateRoute(point, intervals, clear)
        except Exception as e:            
            logging.error(e, exc_info=True)
            #TODO more fine-grained error control            
            pushWarning("Could not compute isochrones")            

    def clearPins(self):
        self.originSearchBox.removePin()

    def addPins(self):
        self.originSearchBox.addPin()

    def actionToggled(self, toggled):
        if toggled:
            self.addPins()
        else:
            self.clearPins()

    def intervalChanges(self):
        try:
            self.getInterval()
            self.lineEditIntervals.setStyleSheet("color: black;")
        except Exception as e:
            pushMessage(str(e))
            self.lineEditIntervals.setStyleSheet("color: red;")

    def getInterval(self):
        """Get interval of as a list of integer. 
        It also make sure that the list is ascending
        """
        intervalText = self.lineEditIntervals.text()
        # remove white space
        intervalText = ''.join(intervalText.split())
        interval = intervalText.split(';')
        # try to convert to int
        interval = [int(x) for x in interval if len(x) > 0]
        # sort it
        return sorted(interval)
