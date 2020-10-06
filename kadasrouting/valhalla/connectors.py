import os
import subprocess
import logging
import requests
import json

from kadasrouting.exceptions import Valhalla400Exception
from kadasrouting.utilities import localeName

LOG = logging.getLogger(__name__)

class Connector:
    def prepareRouteParameters(self, points, profile="auto", options=None):
        options = options or {}
        locale_name = localeName()
        params = dict(
            costing=profile,
            show_locations=True,
            locations=points,
            directions_options={"language": locale_name})
        if options:
            params["costing_options"] = {profile: options}

        return params

    def prepareIsochronesParameters(self, points, profile, options, intervals, colors):
        # build contour json
        if len(intervals) != len(colors):
            LOG.warning(self.tr(
                "The number of intervals and colors are different, using default color"
            ))
            contours = [{"time": x} for x in intervals]
        else:
            contours = []
            for i in range(0, len(intervals)):
                contours.append({"time": intervals[i], "color": colors[i]})
        params = dict(
            costing=profile, locations=points, polygons=True,
            contours=contours, costing_options={profile: options})
        return params

    def prepareMapmatchingParameters(self, shape, profile, options):
        return {"shape": shape,
                "shape_match": "map_snap",
                "costing": profile,
                "costing_options": {profile: options}}


class ConsoleConnector(Connector):
    def _execute(self, action, request):
        valhallaPath = os.path.join(os.path.dirname(os.path.dirname(__file__)), 
                                "executables", "valhalla")
        os.chdir(valhallaPath)
        valhallaExecutable = os.path.join(valhallaPath,  "valhalla_service.exe")
        valhallaConfig = os.path.join(valhallaPath,  "valhalla.json")
        commands = [valhallaExecutable, valhallaConfig, action, request]        
        result = subprocess.run(commands, stdout=subprocess.PIPE, stderr=subprocess.DEVNULL, shell=True)
        response = json.loads(result.stdout.decode("utf-8"))
        if "error" in response:
            raise Exception(response["error"])        
        return response

    def route(self, points, profile, options):
        params = self.prepareRouteParameters(points, profile, options)
        response = self._execute("route", json.dumps(params))
        return response

    def isochrones(self, points, profile, options, intervals, colors):
        params = self.prepareIsochronesParameters(points, profile, options,
                                                  intervals, colors)
        response = self._execute("isochrone", json.dumps(params))
        return response

    def mapmatching(self, shape, profile, options):
        params = self.prepareMapmatchingParameters(shape, profile, options)       
        response = self._execute("trace_route", json.dumps(params))
        return response


class HttpConnector(Connector):
    def __init__(self, url):
        self.url = url

    def _request(self, endpoint, payload):
        url = f"{self.url}/{endpoint}?json={payload}"
        logging.info("Requesting %s" % url)
        response = requests.get(url)
        # Custom handling for Valhalla to raise the detailed message also.
        if response.status_code == 400:
            raise Valhalla400Exception(response.text)
        response.raise_for_status()
        return response.json()

    def route(self, points, profile, options):
        params = self.prepareRouteParameters(points, profile, options)
        response = self._request("route", json.dumps(params))
        return response

    def isochrones(self, points, profile, options, intervals, colors):
        params = self.prepareIsochronesParameters(points, profile, options,
                                                  intervals, colors)
        response = self._request("isochrone", json.dumps(params))
        return response

    def mapmatching(self, shape, profile, options):
        params = self.prepareMapmatchingParameters(shape, profile, options)
        url = f"{self.url}/trace_route"
        response = requests.post(url, json=params)
        if response.status_code == 400:
            raise Valhalla400Exception(response.text)
        response.raise_for_status()
        return response.json()
