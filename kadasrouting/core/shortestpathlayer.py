from PyQt5.QtCore import QTimer, pyqtSignal

from kadas.kadasgui import (
    KadasPinItem, 
    KadasItemPos,   
    KadasItemLayer,
    KadasLineItem)

from kadasrouting.utilities import iconPath, waitcursor, pushMessage

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
from kadas.kadascore import KadasPluginLayerType
from qgisvalhalla.client import ValhallaClient

class RoutePointMapItem(KadasPinItem):

    hasChanged = pyqtSignal()

    def itemName(self):
        return "Route point"

    def edit(self, context, pos, settings):
        super().edit(context, pos, settings)
        self.hasChanged.emit()

class ShortestPathLayer(KadasItemLayer):

    LAYER_TYPE="shortestpath"

    def __init__(self, name):
        KadasItemLayer.__init__(self, name, QgsCoordinateReferenceSystem("EPSG:4326"), ShortestPathLayer.LAYER_TYPE)
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

    def layerType(self):
        return ShortestPathLayer.LAYER_TYPE

    def readXml(self, node):
        KadasItemLayer.readXml(self, node)
        # custom properties
        # self.readImage( node.toElement().attribute("image_path", ".") )
        # self.notes = node.toElement().attribute("notes", ".")
        pushMessage('Notes found: %s' % self.notes)

        return True

    def writeXml(self, node, doc, context):
        KadasItemLayer.writeXml(self, node, doc, context)
        element = node.toElement()
        # write plugin layer type to project  (essential to be read from project)
        element.setAttribute("type", "plugin")
        element.setAttribute("name", self.layerTypeKey())
        pushMessage('Layer name: %s' % self.layerTypeKey())
        self.notes = 'The number of points is %d' % len(self.points)
        element.setAttribute("notes", len(self.notes))
        pushMessage('Notes written: %s' % self.notes)
        # custom properties
        return True

class ShortestPathLayerType(KadasPluginLayerType):

  def __init__(self):
    KadasPluginLayerType.__init__(self, ShortestPathLayer.LAYER_TYPE)

  def createLayer(self):
    return ShortestPathLayer('')

  def showLayerProperties(self, layer):
    return True