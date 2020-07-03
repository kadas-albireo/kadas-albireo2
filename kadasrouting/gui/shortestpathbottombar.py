import os
from PyQt5 import uic
from PyQt5.QtGui import QIcon
from PyQt5.QtWidgets import QMessageBox

from kadas.kadasgui import KadasBottomBar, KadasLayerSelectionWidget
from kadasrouting.gui.locationinputwidget import LocationInputWidget, WrongLocationException
from kadasrouting import vehicles

from qgis.utils import iface
from qgis.core import Qgis, QgsProject, QgsVectorLayer, QgsWkbTypes, QgsLineSymbol, QgsSingleSymbolRenderer

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

        self.layerSelector = KadasLayerSelectionWidget(canvas, iface.layerTreeView(), 
                                                        lambda x: isinstance(x, QgsVectorLayer) 
                                                            and x.geometryType() == QgsWkbTypes.LineGeometry,
                                                        self.createLayer);
        self.layerSelector.createLayerIfEmpty("Route")
        self.layout().addWidget(self.layerSelector, 0, 0, 1, 2)

        self.originSearchBox = LocationInputWidget(canvas)
        self.layout().addWidget(self.originSearchBox, 2, 1)

        self.destinationSearchBox = LocationInputWidget(canvas)
        self.layout().addWidget(self.destinationSearchBox, 3, 1)

        self.waypointsSearchBox = LocationInputWidget(canvas)
        self.layout().addWidget(self.waypointsSearchBox, 4, 1)

        self.comboBoxVehicles.addItems(vehicles.vehicles)

        self.pushButtonClear.clicked.connect(self.clear)
        self.pushButtonReverse.clicked.connect(self.reverse)
        self.btnAddWaypoints.clicked.connect(self.addWaypoints)

    def createLayer(self, name):
        layer = QgsVectorLayer("LineString", name, "memory")
        props = {'line_color': '255,0,0,255', 'line_style': 'solid', 
                'line_width': '2', 'line_width_unit': 'MM'}
        symbol = QgsLineSymbol.createSimple(props)
        renderer = QgsSingleSymbolRenderer(symbol)
        layer.setRenderer(renderer)        
        return layer


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


    def processRouteResult(self, route):
        layer = self.layerSelector.getSelectedLayer()
        provider = layer.dataProvider()
        provider.truncate()
        provider.deleteAttributes(provider.attributeIndexes())
        provider.addFeatures(route.getFeatures())
        layer.updateFields()
        layer.updateExtents()
        layer.triggerRepaint()


    def clear(self):
        self.originSearchBox.clearSearchBox()
        self.destinationSearchBox.clearSearchBox()
        self.waypointsSearchBox.clearSearchBox()
        self.waypoints = []
        self.lineEditWaypoints.clear()

    def addWaypoints(self):
        """Add way point to the list of way points"""
        if self.waypointsSearchBox.text() == '':
            return
        self.waypoints.append(self.waypointsSearchBox.valueAsPoint())
        if self.lineEditWaypoints.text() == '':
            self.lineEditWaypoints.setText(self.waypointsSearchBox.text())
        else:
            self.lineEditWaypoints.setText(self.lineEditWaypoints.text() + ';' + self.waypointsSearchBox.text())
        self.waypointsSearchBox.clearSearchBox()

    def reverse(self):
        """Reverse route"""
        originLocation = self.originSearchBox.text()
        self.originSearchBox.setText(self.destinationSearchBox.text())
        self.destinationSearchBox.setText(originLocation)
        self.waypoints.reverse()
