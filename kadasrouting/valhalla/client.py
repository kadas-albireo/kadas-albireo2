# -*- coding: utf-8 -*-

## Code partially adapted from the QGIS - Valhalla plugin by Nils Nolde(nils@gis-ops.com)

from qgis.core import (QgsPointXY,
                       QgsGeometry,
                       QgsFeature,
                       QgsVectorLayer,
                       QgsField)

from kadasrouting.exceptions import ValhallaException, Valhalla400Exception

from .connectors import HttpConnector

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
            raise ValhallaException(str(e))
        return response

    def isochrones(self, qgspoint, intervals, colors):
        points = self.pointsFromQgsPoints([qgspoint])
        try:
            response = self.connector.isochrones(points, intervals, colors)
        except Valhalla400Exception as e:
            raise e
        except Exception as e:
            raise ValhallaException(str(e))
        return response

    def pointsFromQgsPoints(self, qgspoints):
        points = []
        for qgspoint in qgspoints:     
            points.append({"lon": round(qgspoint.x(), 6), "lat": round(qgspoint.y(), 6)})
        return points


