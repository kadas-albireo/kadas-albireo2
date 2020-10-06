# -*- coding: utf-8 -*-

# Code partially adapted from the QGIS - Valhalla plugin by Nils Nolde(nils@gis-ops.com)

from kadasrouting.exceptions import ValhallaException, Valhalla400Exception
from kadasrouting.utilities import encodePolyline6

from .connectors import ConsoleConnector

class ValhallaClient:
    def __init__(self, connector=None):
        self.connector = connector or ConsoleConnector()

    def route(self, qgspoints, profile, options):
        """
        Computes a route

        :param points: A list of QgsPointsXY in epsg4326 crs with the points that define the route
        :type points: list

        :param profile: the costing profile to use
        :type profile: string

        :param options: The options for computing the route
        :type options: dict

        :returns: Output layer with a single geometry containing the route.
        :rtype: QgsVectorLayer
        """
        points = self.pointsFromQgsPoints(qgspoints)
        try:
            response = self.connector.route(points, profile, options)
        except Exception as e:
            raise ValhallaException(str(e))
        return response

    def isochrones(self, qgspoint, profile, costingOptions, intervals, colors):
        points = self.pointsFromQgsPoints([qgspoint])
        try:
            response = self.connector.isochrones(
                points, profile, costingOptions, intervals, colors)
        except Valhalla400Exception as e:
            raise e
        except Exception as e:
            raise ValhallaException(str(e))
        return response

    def mapmatching(self, line, profile, costingOptions):
        try:
            pt = line[0]
            shape = [{"lat": pt.y(), "lon": pt.x(), "type": "break"}]
            for pt in line[1:-1]:
                shape.append({"lat": pt.y(), "lon": pt.x(), "type": "via"})
            pt = line[-1]
            shape.append({"lat": pt.y(), "lon": pt.x(), "type": "break"})
            response = self.connector.mapmatching(shape, profile, costingOptions)
        except Valhalla400Exception as e:
            raise e
        except Exception as e:
            raise ValhallaException(str(e))
        return response

    def polyline6fromQgsPolylineXY(self, qgsline):
        points = [(p.x(), p.y()) for p in qgsline]
        encoded = encodePolyline6(points)
        return encoded

    def pointsFromQgsPoints(self, qgspoints):
        points = []
        for qgspoint in qgspoints:
            points.append(
                {"lon": round(qgspoint.x(), 6), "lat": round(qgspoint.y(), 6)}
            )
        return points
