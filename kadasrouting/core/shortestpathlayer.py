import logging 

from PyQt5.QtCore import QTimer, pyqtSignal

from kadas.kadasgui import (
    KadasPinItem, 
    KadasItemPos,   
    KadasItemLayer,
    KadasLineItem)

from kadasrouting.utilities import iconPath, waitcursor
from kadasrouting.valhalla.client import ValhallaClient

from qgis.utils import iface
from qgis.core import (
    QgsProject,
    QgsVectorLayer,
    QgsWkbTypes,
    QgsLineSymbol,
    QgsSingleSymbolRenderer,
    QgsCoordinateReferenceSystem,
    QgsCoordinateTransform,
    QgsPointXY
    )


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
        try:
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
        except Exception as e:
            logging.error(e, exc_info=True)
            #TODO more fine-grained error control            
            pushWarning("Could not compute route")
            logging.error("Could not compute route")
        

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
