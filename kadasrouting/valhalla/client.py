# -*- coding: utf-8 -*-

## Code partially adapted from the QGIS - Valhalla plugin by Nils Nolde(nils@gis-ops.com)


from PyQt5.QtCore import QVariant

from qgis.core import (QgsPointXY,
                       QgsGeometry,
                       QgsFeature,
                       QgsVectorLayer,
                       QgsField)

from .utils import transformToWGS, decodePolyline6

from .connectors import HttpConnector

class RoutingException(Exception):
    pass

TEST_URL = "https://valhalla.gis-ops.com/osm"

class ValhallaClient():

    def __init__(self, connector = None):
        self.connector = connector or HttpConnector(TEST_URL)


    def route(self, qgspoints, options, shortest=False):
        """
        Computes a route

        :param points: A list of QgsPointsXY in epsg4326 crs with the points that define the route
        :type points: list

        :param options: The options for computing the route
        :type options: dict

        :param points: If True, computes shortest length route instead of shorter time
        :type points: bool

        :returns: Ouput layer with a single geometry containing the route.
        :rtype: QgsVectorLayer
        """


        points = self.pointsFromQgsPoints(qgspoints)
        try:
            response = self.connector.route(points, options, shortest)
        except Exception as e:
            raise RoutingException(str(e))
        route = self.createRouteFromResponse(response)
        return route, response

    def pointsFromQgsPoints(self, qgspoints):
        points = []
        for qgspoint in qgspoints:     
            points.append({"lon": round(qgspoint.x(), 6), "lat": round(qgspoint.y(), 6)})
        return points


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


