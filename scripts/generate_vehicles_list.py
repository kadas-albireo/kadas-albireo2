# This script is used to generate the list of vehicles for the plugin manual

import os
import csv

vehicles_file = os.path.join('kadasrouting', 'resources', 'vehicles.csv')

with open(vehicles_file) as csvfile:
    reader = csv.DictReader(csvfile)
    for i, row in enumerate(reader):
        print('%s. %s' % (i + 1, row['type_en']))
