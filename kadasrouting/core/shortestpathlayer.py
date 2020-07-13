import json
import logging 

from PyQt5.QtCore import QTimer, pyqtSignal, QVariant

from kadas.kadasgui import (
    KadasPinItem, 
    KadasItemPos,   
    KadasItemLayer,
    KadasLineItem)

from kadasrouting.utilities import (
    iconPath, 
    waitcursor, 
    pushMessage, 
    pushWarning,
    transformToWGS, 
    decodePolyline6)

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
    QgsPointXY,    
    QgsGeometry,
    QgsPointXY,
    QgsGeometry,
    QgsFeature,
    QgsVectorLayer,
    QgsField)


from kadas.kadascore import KadasPluginLayerType

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
        self.lineItem = None
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
            for i, pin in enumerate(self.pins):
                self.points[i] = QgsPointXY(pin.position())
            response = self.valhalla.route(self.points, self.costingOptions, self.shortest)            
            self.computeFromResponse(response)
            self.triggerRepaint()
        except Exception as e:
            logging.error(e, exc_info=True)
            #TODO more fine-grained error control            
            pushWarning("Could not compute route")
            logging.error("Could not compute route")
        

    @waitcursor
    def updateRoute(self, points, costingOptions, shortest):
        response = self.valhalla.route(points, costingOptions, shortest)
        self.costingOptions = costingOptions
        self.shortest = shortest
        self.points = points
        self.computeFromResponse(response)
        self.triggerRepaint()            

    def computeFromResponse(self, response):
        epsg4326 = QgsCoordinateReferenceSystem("EPSG:4326")
        self.clear()
        self.response = response
        route = self.createRouteFromResponse(response)
        feature = list(route.getFeatures())[0]
        self.lineItem = KadasLineItem(epsg4326, True)
        self.lineItem.addPartFromGeometry(feature.geometry().constGet())
        self.lineItem.setTooltip(f"Distance: {feature['DIST_KM']}<br/>Time: {feature['DURATION_H']}")
        self.addItem(self.lineItem)
        for i, pt in enumerate(self.points):
            pin = RoutePointMapItem(epsg4326)
            pin.setPosition(KadasItemPos(pt.x(), pt.y()))
            if i == 0:                                
                pin.setFilePath(iconPath('pin_origin.svg'))
                pin.setName('Origin Point')
            elif i == len(self.points) - 1:
                pin.setFilePath(iconPath('pin_destination.svg'))                
                pin.setName('Destination Point')
            else:                
                pin.setup(':/kadas/icons/waypoint', pin.anchorX(), pin.anchorX(), 32, 32)
                pin.setName('Waypoint %d' % i)
            pin.hasChanged.connect(self.pinHasChanged)
            self.pins.append(pin)
            self.addItem(pin)

    def layerTypeKey(self):
        return ShortestPathLayer.LAYER_TYPE

    def readXml(self, node, context):        
        element = node.toElement()
        response = json.loads(element.attribute("response"))
        points = json.loads(element.attribute("points"))
        self.points = [QgsGeometry.fromWkt(wkt).asPoint() for wkt in points]
        self.costingOptions = json.loads(element.attribute("costingOptions"))
        self.shortest = element.attribute("shortest")
        self.computeFromResponse(response)        
        return True

    def writeXml(self, node, doc, context):
        KadasItemLayer.writeXml(self, node, doc, context)
        element = node.toElement()
        # write plugin layer type to project  (essential to be read from project)
        element.setAttribute("type", "plugin")
        element.setAttribute("name", self.layerTypeKey())
        element.setAttribute("response", json.dumps(self.response))
        element.setAttribute("points", json.dumps([pt.asWkt() for pt in self.points]))
        element.setAttribute("shortest", self.shortest)
        element.setAttribute("costingOptions", json.dumps(self.costingOptions))
        return True


    def createRouteFromResponse(self, response):
        """
        Build output layer based on response attributes for directions endpoint.

        :param response: API response object
        :type response: dict


        :returns: Ouput layer with a single geometry containing the route.
        :rtype: QgsVectorLayer
        """
        response_mini = response['trip']
        feat = QgsFeature()
        coordinates, distance, duration = [], 0, 0
        for leg in response_mini['legs']:
                coordinates.extend([
                    list(reversed(coord))
                    for coord in decodePolyline6(leg['shape'])
                ])
                duration += round(leg['summary']['time'] / 3600, 3)
                distance += round(leg['summary']['length'], 3)

        qgis_coords = [QgsPointXY(x, y) for x, y in coordinates]
        feat.setGeometry(QgsGeometry.fromPolylineXY(qgis_coords))
        feat.setAttributes([distance,
                            duration
                            ])

        layer = QgsVectorLayer("LineString?crs=epsg:4326", "route", "memory")
        provider = layer.dataProvider()
        provider.addAttributes([QgsField("DIST_KM", QVariant.Double),
                             QgsField("DURATION_H", QVariant.Double)])
        layer.updateFields()
        provider.addFeature(feat)
        layer.updateExtents()
        return layer

class ShortestPathLayerType(KadasPluginLayerType):

  def __init__(self):
    KadasPluginLayerType.__init__(self, ShortestPathLayer.LAYER_TYPE)

  def createLayer(self):
    return ShortestPathLayer('')
    
  def showLayerProperties(self, layer):
    return True