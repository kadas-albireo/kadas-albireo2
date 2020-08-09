import subprocess
import logging
from kadasrouting.exceptions import Valhalla400Exception

LOG = logging.getLogger(__name__)


class Connector:
    def prepareRouteParameters(self, points, shortest=False, options=None):
        options = options or {}
        profile = "auto_shorter" if shortest else "auto"

        params = dict(costing=profile, show_locations=True, locations=points)
        params["locations"] = points

        if options:
            params["costing_options"] = {profile: options}

        return params

    def prepareIsochronesParameters(self, points, intervals, colors):
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
            costing="auto", locations=points, polygons=True, contours=contours
        )
        return params


class ConsoleConnector(Connector):
    def _execute(self, commands):
        response = ""
        with subprocess.Popen(
            commands,
            shell=True,
            stdout=subprocess.PIPE,
            stdin=subprocess.DEVNULL,
            stderr=subprocess.STDOUT,
            universal_newlines=True,
        ) as proc:
            try:
                for line in iter(proc.stdout.readline, ""):
                    response += line
            except:
                pass
        responsedict = json.loads(response)

    def route(self, points, shortest, options):
        params = self.prepareRouteParameters(points, shortest, options)
        response = _execute(["valhalla_run_route", "-j", json.dumps(params)])
        return response


import requests
import json


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

    def route(self, points, options, shortest):
        params = self.prepareRouteParameters(points, options, shortest)
        response = self._request("route", json.dumps(params))
        return response

    def isochrones(self, points, intervals, colors):
        params = self.prepareIsochronesParameters(points, intervals, colors)
        response = self._request("isochrone", json.dumps(params))
        return response
