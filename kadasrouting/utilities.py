# -*- coding: utf-8 -*-
import os
import itertools
import io
import math


from io import StringIO
from html.parser import HTMLParser

from PyQt5.QtCore import QLocale, QCoreApplication, QSettings, Qt
from PyQt5.QtGui import QIcon
from PyQt5.QtWidgets import QMessageBox, QApplication

from qgis.utils import iface
from qgis.core import (
    QgsCoordinateReferenceSystem,
    QgsCoordinateTransform,
    QgsProject,
    Qgis,
)


def tr(x):
    return QCoreApplication.translate("", x)


def localeName():
    override_flag = QSettings().value(
        'locale/overrideFlag', True, type=bool)

    if override_flag:
        locale_name = QSettings().value('locale/userLocale', 'en_US', type=str)
    else:
        locale_name = QLocale.system().name()
        locale_name = str(locale_name).split('_')[0]

    return locale_name

def appDataDir():
    folder = os.path.expandvars('%APPDATA%\\KADAS\\routing\\')
    if not os.path.exists(folder):
        os.makedirs(folder)
    return folder

def iconPath(name):
    return os.path.join(os.path.dirname(__file__), "icons", name)


def icon(name):
    return QIcon(iconPath(name))


def showMessageBox(text):
    QMessageBox.information(iface.mainWindow(), "Log", text)


def pushMessage(text):
    iface.messageBar().pushMessage(tr("Info"), text, level=Qgis.Info)


def pushWarning(text):
    iface.messageBar().pushMessage(tr("Warning"), text, level=Qgis.Warning)


def waitcursor(method):
    def func(*args, **kw):
        try:
            QApplication.setOverrideCursor(Qt.WaitCursor)
            return method(*args, **kw)
        except Exception as ex:
            raise ex
        finally:
            QApplication.restoreOverrideCursor()

    return func


def transformToWGS(crs):
    """
    Returns a transformer to WGS84

    :param old_crs: CRS to transform from
    :type old_crs: QgsCoordinateReferenceSystem

    :returns: transformer to use in various modules.
    :rtype: QgsCoordinateTransform
    """
    outCrs = QgsCoordinateReferenceSystem(4326)
    xformer = QgsCoordinateTransform(crs, outCrs, QgsProject.instance())

    return xformer


"""
Copyright (c) 2014 Bruno M. Custódio
Copyright (c) 2016 Frederick Jansen
https://github.com/hicsail/polyline/
"""


class PolylineCodec(object):
    def _pcitr(self, iterable):
        return zip(iterable, itertools.islice(iterable, 1, None))

    def _py2_round(self, x):
        # The polyline algorithm uses Python 2's way of rounding
        return int(math.copysign(math.floor(math.fabs(x) + 0.5), x))

    def _write(self, output, curr_value, prev_value, factor):
        curr_value = self._py2_round(curr_value * factor)
        prev_value = self._py2_round(prev_value * factor)
        coord = curr_value - prev_value
        coord <<= 1
        coord = coord if coord >= 0 else ~coord

        while coord >= 0x20:
            output.write(chr((0x20 | (coord & 0x1f)) + 63))
            coord >>= 5

        output.write(chr(coord + 63))

    def _trans(self, value, index):
        byte, result, shift = None, 0, 0

        while byte is None or byte >= 0x20:
            byte = ord(value[index]) - 63
            index += 1
            result |= (byte & 0x1f) << shift
            shift += 5
            comp = result & 1

        return ~(result >> 1) if comp else (result >> 1), index

    def decode(self, expression, precision=6, geojson=False):
        coordinates, index, lat, lng, length, factor = [], 0, 0, 0, len(expression), float(10 ** precision)

        while index < length:
            lat_change, index = self._trans(expression, index)
            lng_change, index = self._trans(expression, index)
            lat += lat_change
            lng += lng_change
            coordinates.append((lat / factor, lng / factor))

        if geojson is True:
            coordinates = [t[::-1] for t in coordinates]

        return coordinates

    def encode(self, coordinates, precision=6, geojson=False):
        if geojson is True:
            coordinates = [t[::-1] for t in coordinates]

        output, factor = io.StringIO(), int(10 ** precision)

        self._write(output, coordinates[0][0], 0, factor)
        self._write(output, coordinates[0][1], 0, factor)

        for prev, curr in self._pcitr(coordinates):
            self._write(output, curr[0], prev[0], factor)
            self._write(output, curr[1], prev[1], factor)

        return output.getvalue()


def decodePolyline6(expression, precision=6, geojson=False):
    return PolylineCodec().decode(expression, precision, geojson)


def encodePolyline6(coordinates, precision=6, geojson=False):
    return PolylineCodec().encode(coordinates, precision, geojson)


def formatdist(d):
    if d is None:
        return ""
    return "{d:.1f} km".format(d=d/1000) if d > 1000 else "{d:.0f} m".format(d=d)


class MLStripper(HTMLParser):
    def __init__(self):
        super().__init__()
        self.reset()
        self.strict = False
        self.convert_charrefs = True
        self.text = StringIO()

    def handle_data(self, d):
        self.text.write(d)

    def get_data(self):
        return self.text.getvalue()


def strip_tags(html):
    s = MLStripper()
    s.feed(html)
    return s.get_data()
