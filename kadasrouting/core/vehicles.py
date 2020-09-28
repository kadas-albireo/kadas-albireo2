import os
import csv

from kadasrouting.utilities import localeName

_vehicles = []
_vehicles_reduced = []

VEHICLE_REDUCED_NAMES = ["WALK", "BICYCLE", "CAR"]

vehicles_file = os.path.join(os.path.dirname(
            os.path.dirname(__file__)), "resources", "vehicles.csv")

vehicle_name_column = f"type_{localeName()}".lower()

COST_MODEL = "cost_model"
HEIGHT = "height_m"
LENGTH = "length_m"
WIDTH = "width_m"

def read_vehicles():
    global _vehicles
    _vehicles = []
    global _vehicles_reduced
    _vehicles_reduced = []
    with open(vehicles_file, encoding="utf8") as csv_file:
        reader = csv.reader(csv_file)
        first = True
        for row in reader:
            if first:
                columns = [s for s in row]
                first = False
            else:
                _vehicles.append({k:v for k,v in zip(columns, row)})
    _vehicles_reduced = [v for v in _vehicles if v["type_en"] in VEHICLE_REDUCED_NAMES]

def vehicle_names():
    return [v.get(vehicle_name_column, v["type_en"]) for v in _vehicles]

def vehicles():
    return _vehicles

def options_for_vehicle(i):
    vehicle = _vehicles[i]
    profile = vehicle[COST_MODEL]
    costing_options = {}
    if profile == "truck":
        costing_options = {"height": vehicle[HEIGHT],
                           "width": vehicle[WIDTH],
                           "length": vehicle[LENGTH]}
    return profile, costing_options

def vehicle_reduced_names():
    return [v.get(vehicle_name_column, v["type_en"]) for v in _vehicles_reduced]

def vehicles_reduced():
    return _vehicles_reduced

def options_for_vehicle_reduced(i):
    vehicle = _vehicles_reduced[i]
    profile = vehicle[COST_MODEL]
    costing_options = {}    
    return profile, costing_options

read_vehicles()