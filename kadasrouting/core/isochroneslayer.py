import json
import logging

from PyQt5.QtCore import QTextCodec
from PyQt5.QtGui import QColor

from kadasrouting.utilities import waitcursor, tr

from kadasrouting.valhalla.client import ValhallaClient

from qgis.core import (
    QgsProject,
    QgsWkbTypes,
    QgsSingleSymbolRenderer,
    QgsFeature,
    QgsJsonUtils,
    QgsVectorLayer,
    QgsGeometry,
    QgsSvgMarkerSymbolLayer,
    QgsMarkerSymbolLayer
)

LOG = logging.getLogger(__name__)


class OverwriteError(Exception):
    pass


valhalla = ValhallaClient.getInstance()


def getFeaturesFromResponse(response):
    """Return a list of features from a valhalla response object
    """
    codec = QTextCodec.codecForName("UTF-8")
    fields = QgsJsonUtils.stringToFields(json.dumps(response), codec)
    features = QgsJsonUtils.stringToFeatureList(json.dumps(response), fields, codec)
    return features


@waitcursor
def generateIsochrones(point, profile, costingOptions, intervals, colors, basename, overwrite=True):
    response = valhalla.isochrones(point, profile, costingOptions, intervals, colors)
    features = getFeaturesFromResponse(response)
    if costingOptions.get('shortest'):
        suffix = 'km'
    else:
        suffix = 'min'
    for interval, feature in zip(intervals[::-1], features):
        # FIXME: we should use the 'contour' property in the feature to be sure of the contour line that we are
        # drawing, but due to a bug in qgis json parser, this property appears to be always set to '0'
        layername = "{} {} - {}".format(interval, suffix, basename)
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
        fillColor = QColor("#C0" + feature["color"][1:])
        renderer = QgsSingleSymbolRenderer.defaultRenderer(QgsWkbTypes.PolygonGeometry)
        symbol = renderer.symbol()
        symbol.setColor(fillColor)
        symbol.symbolLayer(0).setStrokeColor(outlineColor)
        layer.setRenderer(renderer)

    # Add center of reachability
    center_point_layer_name = tr('Center of {basename}').format(basename=basename)
    try:
        existinglayer = QgsProject.instance().mapLayersByName(center_point_layer_name)[0]
        if overwrite:
            QgsProject.instance().removeMapLayer(existinglayer.id())
        else:
            raise OverwriteError(tr(
                "layer {layername} already exists and overwrite is {overwrite}").format(
                    layername=center_point_layer_name, overwrite=overwrite
                )
            )
    except IndexError:
        LOG.debug("this layer was not found: {}".format(center_point_layer_name))

    center_point = QgsVectorLayer(
        "Point?crs=epsg:4326",
        center_point_layer_name,
        "memory",
    )
    pr = center_point.dataProvider()
    qgsfeature = QgsFeature()
    qgsfeature.setGeometry(QgsGeometry.fromPointXY(point))
    pr.addFeatures([qgsfeature])
    # symbology
    path = ":/kadas/icons/pin_red"
    symbol = QgsSvgMarkerSymbolLayer(path)
    symbol.setSize(10)
    symbol.setVerticalAnchorPoint(QgsMarkerSymbolLayer.Bottom)
    center_point.renderer().symbol().changeSymbolLayer(0, symbol)
    QgsProject.instance().addMapLayer(center_point)
