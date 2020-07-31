import os
import logging
import json
import math

LOG = logging.getLogger(__name__)

from PyQt5 import uic
from PyQt5.QtGui import QIcon, QColor
from PyQt5.QtWidgets import QDesktopWidget, QLineEdit

from kadas.kadasgui import (
    KadasBottomBar,
    KadasPinItem,
    KadasItemPos,
    KadasMapCanvasItemManager,
    KadasLayerSelectionWidget
    )
from kadasrouting.gui.locationinputwidget import LocationInputWidget, WrongLocationException
from kadasrouting import vehicles
from kadasrouting.utilities import iconPath, pushMessage, pushWarning, showMessageBox

from qgis.utils import iface
from qgis.core import (
    Qgis,
    QgsProject,
    QgsCoordinateReferenceSystem,
    QgsCoordinateTransform,
    QgsRectangle
    )

from kadasrouting.core.isochroneslayer import IsochronesLayer, IsochroneLayerGenerator, OverwriteError

from kadasrouting.exceptions import Valhalla400Exception

WIDGET, BASE = uic.loadUiType(os.path.join(os.path.dirname(__file__), 'reachibilitybottombar.ui'))

class ReachibilityBottomBar(KadasBottomBar, WIDGET):

    def __init__(self, canvas, action):
        KadasBottomBar.__init__(self, canvas, "orange")
        self.setupUi(self)
        self.setStyleSheet("QFrame { background-color: orange; }")
        self.action = action
        self.canvas = canvas

        self.btnClose.setIcon(QIcon(":/kadas/icons/close"))
        self.btnClose.setToolTip('Close reachibility dialog')

        self.action.toggled.connect(self.actionToggled)
        self.btnClose.clicked.connect(self.action.toggle)

        self.btnCalculate.clicked.connect(self.calculate)

        self.originSearchBox = LocationInputWidget(canvas, locationSymbolPath=iconPath('blue_cross.svg'))
        self.layout().addWidget(self.originSearchBox, 3, 1)
        # FIXME: I don't know if layerSelector is useful anymore
        self.layerSelector = KadasLayerSelectionWidget(canvas, iface.layerTreeView(),
                                                        lambda x: isinstance(x, IsochronesLayer),
                                                        self.createLayer)

        self.comboBoxVehicles.addItems(vehicles.vehicles)

        self.reachibilityMode = {
            'isochrone': self.tr('Isochrone'),
            'isodistance': self.tr('Isodistance'),
        }

        self.comboBoxReachibiliyMode.addItems(self.reachibilityMode.values())
        self.comboBoxReachibiliyMode.currentIndexChanged.connect(self.setIntervalToolTip)
        self.setIntervalToolTip()

        self.lineEditIntervals.textChanged.connect(self.intervalChanges)
        self.intervalChanges()
        self.lineEditBasename.textChanged.connect(self.basenameChanges)
        self.basenameChanges()

        # Update center of map according to selected point
        self.originSearchBox.searchBox.textChanged.connect(self.centerMap)

        # Always set to center of map for the first time
        self.setCenterAsSelected()

        # Update the point when the canvas extent changed.
        self.canvas.extentsChanged.connect(self.setCenterAsSelected)

        # Handling HiDPI screen, perhaps we can make a ratio of the screen size
        size = QDesktopWidget().screenGeometry()
        if size.width() >= 3200 or size.height() >= 1800:
            self.setFixedSize(self.size().width(), self.size().height() * 1.5)

    def setCenterAsSelected(self, point=None):
        # Set the current center of the map as the selected point
        map_center = self.canvas.center()
        self.originSearchBox.updatePoint(map_center, None)

    def centerMap(self):
        """Pan map so that the current selected point as the center of the map canvas."""
        # Get point from the widget
        point = self.originSearchBox.valueAsPoint()
        # Convert point to canvas CRS
        inCrs = QgsCoordinateReferenceSystem(4326)
        canvasCrs = self.canvas.mapSettings().destinationCrs()
        transform = QgsCoordinateTransform(inCrs, canvasCrs, QgsProject.instance())
        canvasPoint = transform.transform(point)
        # Center the map to the converted point with the same zoom level
        rect = QgsRectangle(canvasPoint, canvasPoint)
        self.canvas.setExtent(rect)
        self.canvas.refresh()

    def createLayer(self, name):
        layer = IsochronesLayer(name)
        return layer

    def calculate(self):
        overwrite = self.checkBoxRemovePrevious.isChecked()
        LOG.debug('isochrones layer name = {}'.format(self.getBasename()))
        try:
            point = self.originSearchBox.valueAsPoint()
        except WrongLocationException as e:
            pushWarning("Invalid location %s" % str(e))
            return
        try:
            intervals = self.getInterval()
            if not (1 <= len(intervals) <= 10):
                raise Exception('Must have at least one and maximum 10 intervals.')
        except Exception as e:
            pushWarning("Invalid intervals: %s" % str(e))
            return
        isochroneLayersGenerator = IsochroneLayerGenerator(self.getBasename())
        colors = []
        try:
            colors = self.getColorFromInterval()
            LOG.debug('_'.join(colors))
            isochroneLayersGenerator.generateIsochrones(point, intervals, colors, overwrite)
        except OverwriteError as e:
            pushWarning("Please change the basename or activate the overwrite checkbox")
        except Valhalla400Exception as e:
            # Expecting the content can be parsed as JSON, see
            # https://valhalla.readthedocs.io/en/latest/api/turn-by-turn/api-reference/#http-status-codes-and-conditions
            json_error = json.loads(str(e))
            pushWarning(
                'Can not generate the error because "%s"' % json_error.get('error'))
        except Exception as e:
            pushWarning("could not generate isochrones")
            raise Exception(e)

    def actionToggled(self, toggled):
        if toggled:
            self.setCenterAsSelected()
        else:
            self.originSearchBox.removePin()

    def basenameChanges(self):
        """Slot when the text on the basename line edit changed.

        It set the text color to red, disable the calculate button,
        and update the calculate button tooltip.
        """
        try:
            basename = self.getBasename()
            if basename == '':
                raise Exception('basename can not be empty')
            self.lineEditBasename.setStyleSheet("color: black;")
            self.btnCalculate.setEnabled(True)
            self.btnCalculate.setToolTip('')
        except Exception as e:
            pushMessage(str(e))
            self.lineEditBasename.setStyleSheet("color: red;")
            self.btnCalculate.setEnabled(False)
            self.btnCalculate.setToolTip('Please make sure the basename is correct.')

    def getBasename(self):
        """Get basename as string
        """
        basenameText = self.lineEditBasename.text()
        return str(basenameText)

    def setBasenameToolTip(self):
        """Set the tool tip for basename line edit based on the current mode."""
        self.lineEditBasename.setToolTip(
                'Set basename for the layer.')

    def intervalChanges(self):
        """Slot when the text on the interval line edit changed.

        It set the text color to red, disable the calculate button,
        and update the calculate button tooltip.
        """
        try:
            interval = self.getInterval()
            if len(interval) == 0:
                raise Exception('Interval can not be empty')
            if len(interval) > 10:
                raise Exception('Interval can not be more than 10')
            self.lineEditIntervals.setStyleSheet("color: black;")
            self.btnCalculate.setEnabled(True)
            self.btnCalculate.setToolTip('')
        except Exception as e:
            pushMessage(str(e))
            self.lineEditIntervals.setStyleSheet("color: red;")
            self.btnCalculate.setEnabled(False)
            self.btnCalculate.setToolTip('Please make sure the interval is correct.')

    def getInterval(self):
        """Get interval of as a list of integer or float based on the current mode.
        It also make sure that the list is ascending
        """
        intervalText = self.lineEditIntervals.text()
        # remove white space
        intervalText = ''.join(intervalText.split())
        interval = intervalText.split(';')
        if self.comboBoxReachibiliyMode.currentText() == self.reachibilityMode['isochrone']:
            # try to convert to int for isochrone
            interval = [int(x) for x in interval if len(x) > 0]
        else:
            # try to convert to float for isodistance
            interval = [float(x) for x in interval if len(x) > 0]
        # sort it
        return sorted(interval)

    def setIntervalToolTip(self):
        """Set the tool tip for interval line edit based on the current mode."""
        if self.comboBoxReachibiliyMode.currentText() == self.reachibilityMode['isochrone']:
            self.lineEditIntervals.setToolTip(
                'Set interval as interger in minutes, separated by ";" symbol')
        else:
            self.lineEditIntervals.setToolTip(
                'Set interval as float in KM, separated by ";" symbol')

    def getColorFromInterval(self):
        num_interval = len(self.getInterval())
        # first generate the value for color based on HSV
        # https://doc.qt.io/qt-5/qcolor.html#the-hsv-color-model
        # starting with green v=120 to red v=0
        if num_interval == 1:
            return [0]
        hsv_value = []
        step = 120 / (num_interval - 1)
        current_value = 0
        while math.ceil(current_value) < 120:
            hsv_value.append(int(current_value))
            current_value += step
        hsv_value.append(120)
        hsv_value.reverse()
        hsv_color = [QColor.fromHsv(h, 255, 204, 255) for h in hsv_value]
        for i in hsv_color:
            LOG.debug(i.getHsv())
            LOG.debug(i.getRgb())
        # get the RGB string format
        rgb_color = [c.name() for c in hsv_color]
        # remove the # char
        rgb_value_valhalla = [c[1:] for c in rgb_color]
        return rgb_value_valhalla
