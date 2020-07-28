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

LOG = logging.getLogger(__name__)

class OverwriteError(Exception):
    pass

class IsochroneLayerGenerator(object):
    def __init__(self, basename, overwrite = True):
        self.setBasename(basename)
        self.valhalla = ValhallaClient()
        self.setOverwrite(overwrite)
        self.crs = QgsCoordinateReferenceSystem("EPSG:4326")

    def setOverwrite(self, overwrite):
        """Set overwrite option as bool
        """
        if not isinstance(overwrite, bool):
            raise TypeError('overwrite parameter is of wrong type. Was: {} , but should be "bool"'.format(type(overwrite)))
        self.overwrite = overwrite

    def getOverwrite(self):
        return self.overwrite

    def setBasename(self, basename):
        """Set basename as string
        """
        self.basename = str(basename)

    def createLayer(self, name):
        layer = IsochronesLayer(name)
        return layer

    def getBasename(self):
        """Get basename as string
        """
        return str(self.basename)

    def setResponse(self, response):
        self.response = response

    def setFeatureToLayer(self, layer, feature):
        item = KadasPolygonItem(self.crs, True)
        item.addPartFromGeometry(feature.geometry().constGet())
        item.setOutline(QPen(QColor(0,0,0)))
        item.setFill(QBrush(QColor(f"#80" + feature["color"][1:])))
        layer.addItem(item)

    @staticmethod
    def getFeaturesFromResponse(response):
        """Return a list of features from a valhalla response object
        """
        fields = QgsFields()
        fields.append(QgsField("opacity", QVariant.Double))
        fields.append(QgsField("fill", QVariant.String))
        fields.append(QgsField("fillOpacity", QVariant.Double))
        fields.append(QgsField("fill-opacity", QVariant.Double))
        fields.append(QgsField("contour", QVariant.Int)) #FIXME: in fact, due to a bug in qgis parser, we cannot use this field
        fields.append(QgsField("color", QVariant.String))
        fields.append(QgsField("fillColor", QVariant.String))        
        codec = QTextCodec.codecForName("UTF-8");
        features = QgsJsonUtils.stringToFeatureList(json.dumps(response), fields, codec)
        # LOG.debug('features : {}'.format(features))
        return features

    @waitcursor
    def generateIsochrones(self, point, intervals, overwrite = True):
        self.setOverwrite(overwrite)
        response = self.valhalla.isochrones(point, intervals)
        features = self.getFeaturesFromResponse(response)
        for feature in features:
            # FIXME: we should use the 'contour' property in the feature to be sure of the contour line that we are
            # drawing, but due to a bug in gqis json parser, this property appears to be always set to '0'
            layername = '{} min - {}'.format(intervals.pop(), self.getBasename())
            try:
                layer =  QgsProject.instance().mapLayersByName(layername)[0] # FIXME: we do not consider if there are several layers with the same name here
                if self.getOverwrite():
                    QgsProject.instance().removeMapLayer( layer.id() )
                else:
                    raise OverwriteError('layer {} already exists and overwrite is {}'.format(layername, self.getOverwrite()))
            except IndexError:
                LOG.debug('this layer was not found: {}'.format(layername))
            layer = self.createLayer(layername)
            QgsProject.instance().addMapLayer(layer)
            layer.clear()
            self.setFeatureToLayer(layer, feature)
            layer.triggerRepaint()


class IsochronesLayer(KadasItemLayer):

    LAYER_TYPE="isochrones"

    def __init__(self, name):
        KadasItemLayer.__init__(self, name, QgsCoordinateReferenceSystem("EPSG:4326"), 
                                IsochronesLayer.LAYER_TYPE)
        self.response = None

    def clear(self):
        items = self.items()
        for itemId in items.keys():
            self.takeItem(itemId)
        self.pins = []

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

  def createLayer(self, name=''):
    return IsochronesLayer(name)
    
  def showLayerProperties(self, layer):
    return True