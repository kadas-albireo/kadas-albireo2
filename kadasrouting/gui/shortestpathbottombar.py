import os
from PyQt5 import uic
from PyQt5.QtGui import QIcon
from PyQt5.QtWidgets import QMessageBox

from kadas.kadasgui import KadasBottomBar
from kadasrouting.gui.locationinputwidget import LocationInputWidget, WrongLocationException
from kadasrouting import vehicles

from qgis.utils import iface
from qgis.core import Qgis, QgsProject

from qgisvalhalla.client import ValhallaClient

WIDGET, BASE = uic.loadUiType(os.path.join(os.path.dirname(__file__), 'shortestpathbottombar.ui'))

class ShortestPathBottomBar(KadasBottomBar, WIDGET):

    def __init__(self, canvas, action):
        KadasBottomBar.__init__(self, canvas, "orange")
        self.setupUi(self)
        self.setStyleSheet("QFrame { background-color: orange; }")
        self.action = action
        self.canvas = canvas
        self.waypoints = []

        self.btnAddWaypoints.setIcon(QIcon(":/kadas/icons/add"))
        self.btnClose.setIcon(QIcon(":/kadas/icons/close"))
        self.btnAddWaypoints.setToolTip('Add waypoint')
        self.btnClose.setToolTip('Close routing dialog')

        self.btnClose.clicked.connect(self.action.toggle)
        self.btnCalculate.clicked.connect(self.calculate)

        self.originSearchBox = LocationInputWidget(canvas)
        self.layout().addWidget(self.originSearchBox, 0, 1)

        self.destinationSearchBox = LocationInputWidget(canvas)
        self.layout().addWidget(self.destinationSearchBox, 1, 1)

        self.waypointsSearchBox = LocationInputWidget(canvas)
        self.layout().addWidget(self.waypointsSearchBox, 2, 1)

        self.comboBoxVehicles.addItems(vehicles.vehicles)

        self.pushButtonClear.clicked.connect(self.clear)
        self.pushButtonReverse.clicked.connect(self.reverse)
        self.btnAddWaypoints.clicked.connect(self.addWaypoints)

    def calculate(self):
        try:
            points = [self.originSearchBox.valueAsPoint()]
            points.extend(self.waypoints)
            points.append(self.destinationSearchBox.valueAsPoint())
        except WrongLocationException as e:
            iface.messageBar().pushMessage("Error", "Invalid location", level=Qgis.Warning)
            return

        shortest = self.radioButtonShortest.isChecked()
        '''
        vehicle = self.comboBoxVehicle.currentIndex()
        costingOptions = vehicles.options[vehicle]
        '''
        costingOptions = {}
        valhalla = ValhallaClient()
        try:
            route = valhalla.route(points, costingOptions, shortest)
            self.processRouteResult(route)
        except:
            #TODO more fine-grained error control
            raise
            iface.messageBar().pushMessage("Error", "Could not compute route", level=Qgis.Warning)


    def processRouteResult(self, route):
        # TODO: process layer and maybe use a custom plugin layer class.
        # Also, maybe use KadasLayerSelectionWidget, as used in similar
        # functionality
        QgsProject.instance().addMapLayer(route)

    def clear(self):
        self.originSearchBox.searchBox.clearSearch()
        self.destinationSearchBox.searchBox.clearSearch()
        self.waypointsSearchBox.searchBox.clearSearch()
        self.waypoints = []
        self.lineEditWaypoints.clear()

    def addWaypoints(self):
        """Add way point to the list of way points"""
        self.waypoints.append(self.waypointsSearchBox.valueAsPoint())
        self.lineEditWaypoints.setText(self.lineEditWaypoints.text() + ', ' + self.waypointsSearchBox.text())
        self.waypointsSearchBox.clearSearch()

    def reverse(self):
        """Reverse the origin and destination"""
        #TODO: implement this
        pass
    