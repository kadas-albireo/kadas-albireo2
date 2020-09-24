import json
import logging

from PyQt5.QtCore import QVariant, QTextCodec
from PyQt5.QtGui import QColor

from kadasrouting.utilities import waitcursor, tr

from kadasrouting.valhalla.client import ValhallaClient

from qgis.core import (
    QgsProject,
    QgsWkbTypes,
    QgsSingleSymbolRenderer,
    QgsFeature,
    QgsJsonUtils,
    QgsFields,
    QgsField,
    QgsVectorLayer
)

LOG = logging.getLogger(__name__)


class OverwriteError(Exception):
    pass


valhalla = ValhallaClient()


def getFeaturesFromResponse(response):
    """Return a list of features from a valhalla response object
    """
    fields = QgsFields()
    fields.append(QgsField("opacity", QVariant.Double))
    fields.append(QgsField("fill", QVariant.String))
    fields.append(QgsField("fillOpacity", QVariant.Double))
    fields.append(QgsField("fill-opacity", QVariant.Double))
    # FIXME: in fact, due to a bug in qgis parser, we cannot use this field
    fields.append(QgsField("contour", QVariant.Int))
    fields.append(QgsField("color", QVariant.String))
    fields.append(QgsField("fillColor", QVariant.String))
    codec = QTextCodec.codecForName("UTF-8")
    features = QgsJsonUtils.stringToFeatureList(json.dumps(response), fields, codec)
    # LOG.debug('features : {}'.format(features))
    return features


@waitcursor
def generateIsochrones(point, profile, costingOptions, intervals, colors, basename, overwrite=True):
    response = valhalla.isochrones(point, profile, costingOptions, intervals, colors)
    features = getFeaturesFromResponse(response)
    for interval, feature in zip(intervals[::-1], features):
        # FIXME: we should use the 'contour' property in the feature to be sure of the contour line that we are
        # drawing, but due to a bug in qgis json parser, this property appears to be always set to '0'
        layername = "{} min - {}".format(interval, basename)
        try:
            # FIXME: we do not consider if there are several layers with the same name here
            existinglayer = QgsProject.instance().mapLayersByName(layername)[0]
            if overwrite:
                QgsProject.instance().removeMapLayer(existinglayer.id())
            else:
                raise OverwriteError(tr(
                    "layer {layername} already exists and overwrite is {overwrite}").format(
                        layername=layername, overwrite=overwrite
                    )
                )
        except IndexError:
            LOG.debug("this layer was not found: {}".format(layername))

        layer = QgsVectorLayer(
            "Polygon?crs=epsg:4326&field=centerx:double&field=centery:double&field=interval:double",
            layername,
            "memory",
        )
        pr = layer.dataProvider()
        qgsfeature = QgsFeature()
        qgsfeature.setAttributes([point.x(), point.y(), interval])
        qgsfeature.setGeometry(feature.geometry())
        pr.addFeatures([qgsfeature])
        layer.updateExtents()
        QgsProject.instance().addMapLayer(layer)
        outlineColor = QColor(0, 0, 0)
        # Set opacity to 75% or C0 in hex (25% transparency)
        fillColor = QColor(f"#C0" + feature["color"][1:])
        renderer = QgsSingleSymbolRenderer.defaultRenderer(QgsWkbTypes.PolygonGeometry)
        symbol = renderer.symbol()
        symbol.setColor(fillColor)
        symbol.symbolLayer(0).setStrokeColor(outlineColor)
        layer.setRenderer(renderer)
