import os
from PyQt5 import uic
from PyQt5.QtGui import QIcon
from PyQt5.QtWidgets import QMessageBox

from kadas.kadasgui import (
    KadasBottomBar, KadasPinItem, KadasItemPos, KadasMapCanvasItemManager)
from kadasrouting.gui.locationinputwidget import LocationInputWidget, WrongLocationException
from kadasrouting import vehicles
from kadasrouting.utilities import iconPath

from qgis.utils import iface
from qgis.core import Qgis, QgsProject, QgsCoordinateReferenceSystem

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
        self.waypointPins = []

        self.btnAddWaypoints.setIcon(QIcon(":/kadas/icons/add"))
        self.btnClose.setIcon(QIcon(":/kadas/icons/close"))
        self.btnAddWaypoints.setToolTip('Add waypoint')
        self.btnClose.setToolTip('Close routing dialog')

        self.btnClose.clicked.connect(self.action.toggle)
        self.btnCalculate.clicked.connect(self.calculate)

        self.originSearchBox = LocationInputWidget(canvas, locationSymbolPath=iconPath('pin_origin.svg'))
        self.layout().addWidget(self.originSearchBox, 0, 1)

        self.destinationSearchBox = LocationInputWidget(canvas, locationSymbolPath=iconPath('pin_destination.svg'))
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
            iface.messageBar().pushMessage("Error", "Invalid location %s" % str(e), level=Qgis.Warning)
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
            iface.messageBar().pushMessage("Error", "Could not compute route", level=Qgis.Warning)
            raise


    def processRouteResult(self, route):
        # TODO: process layer and maybe use a custom plugin layer class.
        # Also, maybe use KadasLayerSelectionWidget, as used in similar
        # functionality
        QgsProject.instance().addMapLayer(route)

    def clear(self):
        self.originSearchBox.clearSearchBox()
        self.destinationSearchBox.clearSearchBox()
        self.waypointsSearchBox.clearSearchBox()
        self.waypoints = []
        self.lineEditWaypoints.clear()
        for waypointPin in self.waypointPins:
            KadasMapCanvasItemManager.removeItem(waypointPin)
        self.waypointPins = []

    def addWaypoints(self):
        """Add way point to the list of way points"""
        if self.waypointsSearchBox.text() == '':
            return
        waypoint = self.waypointsSearchBox.valueAsPoint()
        self.waypoints.append(waypoint)
        if self.lineEditWaypoints.text() == '':
            self.lineEditWaypoints.setText(self.waypointsSearchBox.text())
        else:
            self.lineEditWaypoints.setText(self.lineEditWaypoints.text() + ';' + self.waypointsSearchBox.text())
        self.waypointsSearchBox.clearSearchBox()
        # Remove way point pin and create new one with another symbology
        self.waypointsSearchBox.removePin()

        # Create pin with waypoint symbology
        canvasCrs = QgsCoordinateReferenceSystem(4326)
        waypointPin = KadasPinItem(canvasCrs)
        waypointPin.setPosition(KadasItemPos(waypoint.x(), waypoint.y()))
        waypointPin.setFilePath(':/kadas/icons/waypoint')
        self.waypointPins.append(waypointPin)
        KadasMapCanvasItemManager.addItem(waypointPin)
        
    def reverse(self):
        """Reverse route"""
        originLocation = self.originSearchBox.text()
        self.originSearchBox.setText(self.destinationSearchBox.text())
        self.destinationSearchBox.setText(originLocation)
        self.waypoints.reverse()
        self.waypointPins.reverse()
        waypointsCoordinates = []
        for waypoint in self.waypoints:
            waypointsCoordinates.append('%f, %f' % (waypoint.x(), waypoint.y()))
        self.lineEditWaypoints.setText(';'.join(waypointsCoordinates))
