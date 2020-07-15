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
from kadasrouting.utilities import iconPath, pushMessage, pushWarning, showMessageBox

from qgis.utils import iface
from qgis.core import (
    Qgis,
    QgsProject,
    QgsCoordinateReferenceSystem,
    )

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

        self.originSearchBox = LocationInputWidget(canvas, locationSymbolPath=iconPath('blue_cross.svg'))
        self.layout().addWidget(self.originSearchBox, 2, 1)

        self.comboBoxVehicles.addItems(vehicles.vehicles)

        self.reachibilityMode = {
            'isochrone': self.tr('Isochrone'),
            'isodistance': self.tr('Isodistance'),
        }

        self.comboBoxReachibiliyMode.addItems(self.reachibilityMode.values())
        self.comboBoxReachibiliyMode.currentIndexChanged.connect(self.setIntervalToolTip)

        self.lineEditIntervals.textChanged.connect(self.intervalChanges)
        self.setIntervalToolTip()

        # Always set to center of map
        self.setCenter()

        # Handling HiDPI screen, perhaps we can make a ratio of the screen size
        # size = QDesktopWidget().screenGeometry()
        # if size.width() >= 3200 or size.height() >= 1800:
        #     self.setFixedSize(self.size() / 1.5)

    def setCenter(self):
        map_center = self.canvas.center()
        self.originSearchBox.updatePoint(map_center, None)

    def createLayer(self, name):
        pushMessage('create layer')

    def calculate(self):
        pushMessage('Calculating reachibility')
        text = 'Calculating reachibility\n'
        text += 'Interval is %s\n' % '  -- '.join([str(x) for x in self.getInterval()])
        text += 'Mode is %s\n' % self.comboBoxReachibiliyMode.currentText()
        center_point = self.originSearchBox.valueAsPoint()
        text += 'Center is (%f, %f)\n' % (center_point.x(), center_point.y())
        showMessageBox(text)

    def actionToggled(self, toggled):
        pushMessage('action toggle')
        if toggled:
            self.setCenter()
        else:
            self.originSearchBox.removePin()

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
        if self.comboBoxReachibiliyMode.currentText() == self.reachibilityMode['isochrone']:
            # try to convert to int for isochrone
            interval = [int(x) for x in interval if len(x) > 0]
        else:
            # try to convert to float for isodistance
            interval = [float(x) for x in interval if len(x) > 0]
        # sort it
        return sorted(interval)

    def setIntervalToolTip(self):
        if self.comboBoxReachibiliyMode.currentText() == self.reachibilityMode['isochrone']:
            self.lineEditIntervals.setToolTip(
                'Set interval as interger in minutes, separated by ";" symbol')
        else:
            self.lineEditIntervals.setToolTip(
                'Set interval as float in KM, separated by ";" symbol')
