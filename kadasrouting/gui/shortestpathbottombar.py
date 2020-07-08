import os
from PyQt5 import uic
from PyQt5.QtGui import QIcon
from PyQt5.QtCore import QTimer, pyqtSignal
from PyQt5.QtWidgets import QMessageBox, QDesktopWidget

from kadas.kadasgui import (
    KadasBottomBar, 
    KadasPinItem, 
    KadasItemPos, 
    KadasMapCanvasItemManager, 
    KadasLayerSelectionWidget, 
    KadasItemLayer,
    KadasLineItem)
from kadasrouting.gui.locationinputwidget import LocationInputWidget, WrongLocationException
from kadasrouting import vehicles
from kadasrouting.utilities import iconPath, pushMessage, pushWarning, waitcursor

from qgis.utils import iface
from qgis.core import (
    Qgis,
    QgsProject,
    QgsVectorLayer,
    QgsWkbTypes,
    QgsLineSymbol,
    QgsSingleSymbolRenderer,
    QgsCoordinateReferenceSystem,
    QgsCoordinateTransform,
    QgsPointXY
    )

from qgisvalhalla.client import ValhallaClient

WIDGET, BASE = uic.loadUiType(os.path.join(os.path.dirname(__file__), 'shortestpathbottombar.ui'))

class RoutePointMapItem(KadasPinItem):

    hasChanged = pyqtSignal()

    def itemName(self):
        return "Route point"

    def edit(self, context, pos, settings):
        super().edit(context, pos, settings)
        self.hasChanged.emit()

class ShortestPathLayer(KadasItemLayer):

    def __init__(self, name):
        KadasItemLayer.__init__(self, name, QgsCoordinateReferenceSystem("EPSG:4326"))
        self.response = None
        self.points = []
        self.pins = []
        self.shortest = False
        self.costingOptions = {}        
        self.valhalla = ValhallaClient()
        self.timer = QTimer()
        self.timer.setSingleShot(True)
        self.timer.timeout.connect(self.updateFromPins)

    def setResponse(self, response):
        self.response = response

    def clear(self):
        items = self.items()
        for itemId in items.keys():
            self.takeItem(itemId)
        self.pins = []
    
    def pinHasChanged(self):
        self.timer.start(1000)

    @waitcursor
    def updateFromPins(self):
        outCrs = QgsCoordinateReferenceSystem(4326)
        canvasCrs = iface.mapCanvas().mapSettings().destinationCrs()
        transform = QgsCoordinateTransform(canvasCrs, outCrs, QgsProject.instance())
        for i, pin in enumerate(self.pins):            
            wgspoint = transform.transform(QgsPointXY(pin.position()))
            self.points[i] = wgspoint
        route, response = self.valhalla.route(self.points, self.costingOptions, self.shortest)
        self.response = response
        feature = list(route.getFeatures())[0]
        self.lineItem.clear()
        self.lineItem.addPartFromGeometry(feature.geometry().constGet())
        self.lineItem.setTooltip(f"Distance: {feature['DIST_KM']}<br/>Time: {feature['DURATION_H']}")
        self.triggerRepaint()
        

    @waitcursor
    def updateRoute(self, points, costingOptions, shortest):
        route, response = self.valhalla.route(points, costingOptions, shortest)
        self.clear()
        self.response = response
        self.costingOptions = costingOptions
        self.shortest = shortest
        self.points = points
        feature = list(route.getFeatures())[0]
        self.lineItem = KadasLineItem(QgsCoordinateReferenceSystem("EPSG:4326"), True)
        self.lineItem.addPartFromGeometry(feature.geometry().constGet())
        self.lineItem.setTooltip(f"Distance: {feature['DIST_KM']}<br/>Time: {feature['DURATION_H']}")
        self.addItem(self.lineItem)
        inCrs = QgsCoordinateReferenceSystem(4326)
        canvasCrs = iface.mapCanvas().mapSettings().destinationCrs()
        transform = QgsCoordinateTransform(inCrs, canvasCrs, QgsProject.instance())
        for i, pt in enumerate(points):
            canvasPoint = transform.transform(pt)
            pin = RoutePointMapItem(canvasCrs)            
            pin.setPosition(KadasItemPos(canvasPoint.x(), canvasPoint.y()))
            if i == 0:                                
                pin.setFilePath(iconPath('pin_origin.svg'))
                pin.setName('Origin Point')
            elif i == len(points) - 1:
                pin.setFilePath(iconPath('pin_destination.svg'))                
                pin.setName('Destination Point')
            else:                
                pin.setup(':/kadas/icons/waypoint', pin.anchorX(), pin.anchorX(), 32, 32)
                pin.setName('Waypoint %d' % i)
            pin.hasChanged.connect(self.pinHasChanged)
            self.pins.append(pin)
            self.addItem(pin)
        self.triggerRepaint()            

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

        self.action.toggled.connect(self.actionToggled)
        self.btnClose.clicked.connect(self.action.toggle)

        self.btnCalculate.clicked.connect(self.calculate)

        self.layerSelector = KadasLayerSelectionWidget(canvas, iface.layerTreeView(),
                                                        lambda x: isinstance(x, ShortestPathLayer),
                                                        self.createLayer)
        #self.layerSelector.selectedLayerChanged.connect(self.selectedLayerChanged)
        self.layerSelector.createLayerIfEmpty("Route")
        self.layout().addWidget(self.layerSelector, 0, 0, 1, 2)

        self.originSearchBox = LocationInputWidget(canvas, locationSymbolPath=iconPath('pin_origin.svg'))
        self.layout().addWidget(self.originSearchBox, 2, 1)

        self.destinationSearchBox = LocationInputWidget(canvas, locationSymbolPath=iconPath('pin_destination.svg'))
        self.layout().addWidget(self.destinationSearchBox, 3, 1)

        self.waypointsSearchBox = LocationInputWidget(canvas)
        self.groupBox.layout().addWidget(self.waypointsSearchBox, 0, 0)

        self.comboBoxVehicles.addItems(vehicles.vehicles)

        self.pushButtonClear.clicked.connect(self.clear)
        self.pushButtonReverse.clicked.connect(self.reverse)
        self.btnAddWaypoints.clicked.connect(self.addWaypoints)

        # Handling HiDPI screen, perhaps we can make a ratio of the screen size
        size = QDesktopWidget().screenGeometry()
        if size.width() >= 3200 or size.height() >= 1800:
            self.setFixedSize(self.size() * 1.5)

    def createLayer(self, name):
        layer = ShortestPathLayer(name)
        return layer

    def calculate(self):
        layer = self.layerSelector.getSelectedLayer()
        if layer is None:
            pushWarning("Please, select a valid destination layer")
            return
        try:
            points = [self.originSearchBox.valueAsPoint()]
            points.extend(self.waypoints)
            points.append(self.destinationSearchBox.valueAsPoint())
        except WrongLocationException as e:
            pushWarning("Invalid location %s" % str(e))
            return

        shortest = self.radioButtonShortest.isChecked()
        '''
        vehicle = self.comboBoxVehicle.currentIndex()
        costingOptions = vehicles.options[vehicle]
        '''
        costingOptions = {}
        try:
            layer.updateRoute(points, costingOptions, shortest)
        except:
            raise
            #TODO more fine-grained error control
            pushWarning("Could not compute route")

    def clear(self):
        self.originSearchBox.clearSearchBox()
        self.destinationSearchBox.clearSearchBox()
        self.waypointsSearchBox.clearSearchBox()
        self.waypoints = []
        self.lineEditWaypoints.clear()
        for waypointPin in self.waypointPins:
            KadasMapCanvasItemManager.removeItem(waypointPin)
        self.waypointPins = []
        KadasMapCanvasItemManager.removeItem(self.originSearchBox.pin)
        KadasMapCanvasItemManager.removeItem(self.destinationSearchBox.pin)

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
        # Remove way point pin from the location input widget
        self.waypointsSearchBox.removePin()
        # Create/add new waypoint pin for the waypoint
        self.addWaypointPin(waypoint)

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
        self.clearPins()
        self.addPins()

    def addWaypointPin(self, waypoint):
        """Create a new pin for a waypoint with its symbology"""
        # Create pin with waypoint symbology
        canvasCrs = QgsCoordinateReferenceSystem(4326)
        waypointPin = KadasPinItem(canvasCrs)
        waypointPin.setPosition(KadasItemPos(waypoint.x(), waypoint.y()))
        waypointPin.setup(':/kadas/icons/waypoint', waypointPin.anchorX(), waypointPin.anchorX(), 32, 32)
        self.waypointPins.append(waypointPin)
        KadasMapCanvasItemManager.addItem(waypointPin)

    def clearPins(self):
        """Remove all pins from the map
        Not removing the point stored.
        """
        # remove origin pin
        self.originSearchBox.removePin()
        # remove destination poin
        self.destinationSearchBox.removePin()
        # remove waypoint pins
        for waypointPin in self.waypointPins:
            KadasMapCanvasItemManager.removeItem(waypointPin)

    def addPins(self):
        """Add pins for all stored points."""
        self.originSearchBox.addPin()
        self.destinationSearchBox.addPin()
        for waypoint in self.waypoints:
            self.addWaypointPin(waypoint)

    def actionToggled(self, toggled):
        if toggled:
            self.addPins()
        else:
            self.clearPins()