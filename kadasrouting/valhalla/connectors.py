import subprocess
import logging

class Connector():

    def prepareRouteParameters(self, points, shortest = False, options = None):
        options = options or {}
        profile = "auto_shorter" if shortest else "auto"

        params = dict(
            costing = profile,
            show_locations = True,
            locations = points
        )
        params['locations'] = points

        if options:
            params['costing_options'] = {profile: options}

        return params

    def prepareIsochronesParameters(self, points, intervals):
        params = dict(
            costing = "auto",
            locations = points,
            polygons = True,
            contours = [{"time": x} for x in intervals]
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
                for line in iter(proc.stdout.readline, ''):
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
        logging.info('Requesting %s' % url)
        response = requests.get(url)
        response.raise_for_status()
        return response.json()

    def route(self, points, options, shortest):
        params = self.prepareRouteParameters(points, options, shortest)
        response = self._request("route", json.dumps(params))
        return response

    def isochrones(self, points, intervals):        
        params = self.prepareIsochronesParameters(points, intervals)
        response = self._request("isochrone", json.dumps(params))
        return response
