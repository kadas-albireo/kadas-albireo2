import json
import logging 

from PyQt5.QtCore import QTimer, pyqtSignal, QVariant, QTextCodec
from PyQt5.QtGui import QColor, QPen, QBrush

from kadas.kadasgui import (
    KadasPinItem, 
    KadasItemPos,   
    KadasItemLayer,
    KadasPolygonItem)

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
    QgsField,
    QgsJsonUtils,
    QgsField,
    QgsFields)


from kadas.kadascore import KadasPluginLayerType


class IsochronesLayer(KadasItemLayer):

    LAYER_TYPE="isochrones"

    def __init__(self, name):
        KadasItemLayer.__init__(self, name, QgsCoordinateReferenceSystem("EPSG:4326"), 
                                IsochronesLayer.LAYER_TYPE)
        self.response = None
        self.valhalla = ValhallaClient()


    def setResponse(self, response):
        self.response = response

    def clear(self):
        items = self.items()
        for itemId in items.keys():
            self.takeItem(itemId)
        self.pins = []

    @waitcursor
    def updateRoute(self, point, intervals, clear):
        response = self.valhalla.isochrones(point, intervals)
        self.computeFromResponse(response, clear)
        self.triggerRepaint()            

    def computeFromResponse(self, response, clear):
        epsg4326 = QgsCoordinateReferenceSystem("EPSG:4326")
        if clear:
            self.clear()
        self.response = response
        fields = QgsFields()
        fields.append(QgsField("opacity", QVariant.Double))
        fields.append(QgsField("fill", QVariant.String))
        fields.append(QgsField("fillOpacity", QVariant.Double))
        fields.append(QgsField("fill-opacity", QVariant.Double))
        fields.append(QgsField("contour", QVariant.Int))
        fields.append(QgsField("color", QVariant.String))
        fields.append(QgsField("fillColor", QVariant.String))        
        codec = QTextCodec.codecForName("UTF-8");
        features = QgsJsonUtils.stringToFeatureList(json.dumps(response), fields, codec)
        for feature in features:
            item = KadasPolygonItem(epsg4326, True)
            item.addPartFromGeometry(feature.geometry().constGet())
            item.setOutline(QPen(QColor(0,0,0)))
            item.setFill(QBrush(QColor(f"#80" + feature["color"][1:])))
            self.addItem(item)

    def layerTypeKey(self):
        return IsochronesLayer.LAYER_TYPE

    def readXml(self, node, context):        
        element = node.toElement()
        response = json.loads(element.attribute("response"))
        self.computeFromResponse(response)        
        return True

    def writeXml(self, node, doc, context):
        KadasItemLayer.writeXml(self, node, doc, context)
        element = node.toElement()
        # write plugin layer type to project  (essential to be read from project)
        element.setAttribute("type", "plugin")
        element.setAttribute("name", self.layerTypeKey())
        element.setAttribute("response", json.dumps(self.response))
        return True

class IsochronesLayerType(KadasPluginLayerType):

  def __init__(self):
    KadasPluginLayerType.__init__(self, IsochronesLayer.LAYER_TYPE)

  def createLayer(self):
    return IsochronesLayer('')
    
  def showLayerProperties(self, layer):
    return True