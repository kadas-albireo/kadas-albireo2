import os
import logging
import json

from PyQt5 import uic
from PyQt5.QtGui import QIcon
from PyQt5.QtWidgets import QDesktopWidget

from kadas.kadasgui import (
    KadasBottomBar
)

from kadasrouting.gui.locationinputwidget import (
    LocationInputWidget,
    WrongLocationException
)
from kadasrouting.core import vehicles
from kadasrouting.utilities import (
    iconPath,
    pushMessage,
    pushWarning
)

from qgis.core import (
    QgsProject,
    QgsCoordinateReferenceSystem,
    QgsCoordinateTransform,
    QgsRectangle
)

from kadasrouting.core.isochroneslayer import (
    generateIsochrones,
    OverwriteError
)

from kadasrouting.exceptions import Valhalla400Exception

LOG = logging.getLogger(__name__)

WIDGET, BASE = uic.loadUiType(
    os.path.join(os.path.dirname(__file__), "reachabilitybottombar.ui")
)


class ReachabilityBottomBar(KadasBottomBar, WIDGET):
    def __init__(self, canvas, action):
        KadasBottomBar.__init__(self, canvas, "orange")
        self.setupUi(self)
        self.setStyleSheet("QFrame { background-color: orange; }")
        self.action = action
        self.canvas = canvas

        self.btnClose.setIcon(QIcon(":/kadas/icons/close"))
        self.btnClose.setToolTip(self.tr("Close reachability dialog"))

        self.action.toggled.connect(self.actionToggled)
        self.btnClose.clicked.connect(self.action.toggle)

        self.btnCalculate.clicked.connect(self.calculate)

        # Set the anchor to 0.5, 0.5 because the center of the cross needs to be in the position
        self.originSearchBox = LocationInputWidget(
            canvas, locationSymbolPath=iconPath("blue_cross.svg"), pinAnchorX=0.5, pinAnchorY=0.5
        )
        self.layout().addWidget(self.originSearchBox, 0, 1)

        self.comboBoxVehicles.addItems(vehicles.vehicle_names())

        self.reachabilityMode = {
            "isochrone": self.tr("Isochrone"),
            "isodistance": self.tr("Isodistance"),
        }

        self.comboBoxReachabilityMode.addItems(self.reachabilityMode.values())
        self.comboBoxReachabilityMode.currentIndexChanged.connect(
            self.setIntervalToolTip
        )
        self.setIntervalToolTip()

        self.lineEditIntervals.textChanged.connect(self.intervalChanges)
        self.intervalChanges()
        self.lineEditBasename.textChanged.connect(self.basenameChanges)
        self.basenameChanges()

        # Update center of map according to selected point
        self.originSearchBox.pointUpdated.connect(self.centerMap)

        # Update the point when the canvas extent changed.
        self.canvas.extentsChanged.connect(self.setCenterAsSelected)

        # Always set to center of map for the first time
        self.setCenterAsSelected()

        # Handling HiDPI screen, perhaps we can make a ratio of the screen size
        size = QDesktopWidget().screenGeometry()
        if size.width() >= 3200 or size.height() >= 1800:
            self.setFixedSize(self.size().width(), self.size().height() * 1.5)

    def setCenterAsSelected(self, point=None):
        """Set the current center of the map as the selected point"""
        map_center = self.canvas.center()
        self.originSearchBox.updatePoint(map_center, None)

    def centerMap(self):
        """Pan map so that the current selected point as the center of the map canvas."""
        # Get point from the widget
        point = self.originSearchBox.point

        # Convert point to canvas CRS
        inCrs = QgsCoordinateReferenceSystem(4326)
        canvasCrs = self.canvas.mapSettings().destinationCrs()
        transform = QgsCoordinateTransform(inCrs, canvasCrs, QgsProject.instance())
        canvasPoint = transform.transform(point)

        # First, disconnect the signal to avoid infinite circular function calling
        try:
            self.canvas.extentsChanged.disconnect(self.setCenterAsSelected)
        except TypeError as e:
            LOG.debug(e)

        # Center the map to the converted point with the same zoom level
        rect = QgsRectangle(canvasPoint, canvasPoint)
        self.canvas.setExtent(rect)
        self.canvas.refresh()

        # Reconnect the signal after recentering the map
        self.canvas.extentsChanged.connect(self.setCenterAsSelected)

    def calculate(self):
        overwrite = self.checkBoxRemovePrevious.isChecked()
        LOG.debug("isochrones layer name = {}".format(self.getBasename()))
        try:
            point = self.originSearchBox.point
        except WrongLocationException as e:
            pushWarning(self.tr("Invalid location: {error_message}").format(error_message=str(e)))
            return
        try:
            intervals = self.getInterval()
            if not (1 <= len(intervals) <= 10):
                raise Exception(self.tr("Must have at least one and maximum 10 intervals."))
        except Exception as e:
            pushWarning("Invalid intervals: %s" % str(e))
            return

        vehicle = self.comboBoxVehicles.currentIndex()
        profile, costingOptions = vehicles.options_for_vehicle(vehicle)

        is_isodistance = self.comboBoxReachabilityMode.currentText() == self.reachabilityMode["isodistance"]
        costingOptions['shortest'] = is_isodistance

        colors = []
        try:
            colors = self.getColorFromInterval()
            generateIsochrones(
                point, profile, costingOptions, intervals, colors, self.getBasename(), overwrite)
        except OverwriteError as e:
            LOG.error(e)
            pushWarning(self.tr('Please change the basename or activate the overwrite checkbox'))
        except Valhalla400Exception as e:
            # Expecting the content can be parsed as JSON, see
            # https://valhalla.readthedocs.io/en/latest/api/turn-by-turn/api-reference/#http-status-codes-and-conditions
            json_error = json.loads(str(e))
            pushWarning(
                self.tr('Can not generate the error because "{error_message}"').format(
                    error_message=json_error.get("error"))
            )
        except Exception as e:
            pushWarning("could not generate isochrones")
            raise Exception(e)

    def actionToggled(self, toggled):
        if toggled:
            self.setCenterAsSelected()
            # Update the point when the canvas extent changed.
            self.canvas.extentsChanged.connect(self.setCenterAsSelected)
            self.originSearchBox.pointUpdated.connect(self.centerMap)
        else:
            self.originSearchBox.removePin()
            # Disconnect the signal to avoid the blue cross shown up
            try:
                self.canvas.extentsChanged.disconnect(self.setCenterAsSelected)
            except TypeError as e:
                LOG.debug('self.canvas.extentsChanged.disconnect(self.setCenterAsSelected) %s' % e)
            try:
                self.originSearchBox.pointUpdated.disconnect(self.centerMap)
            except TypeError as e:
                LOG.debug('self.originSearchBox.pointUpdated.disconnect(self.centerMap) %s' % e)

    def basenameChanges(self):
        """Slot when the text on the basename line edit changed.

        It set the text color to red, disable the calculate button,
        and update the calculate button tooltip.
        """
        try:
            basename = self.getBasename()
            if basename == "":
                raise Exception(self.tr("basename can not be empty"))
            self.lineEditBasename.setStyleSheet("color: black;")
            self.btnCalculate.setEnabled(True)
            self.btnCalculate.setToolTip("")
        except Exception as e:
            pushMessage(str(e))
            self.lineEditBasename.setStyleSheet("color: red;")
            self.btnCalculate.setEnabled(False)
            self.btnCalculate.setToolTip(self.tr("Please make sure the basename is correct."))

    def getBasename(self):
        """Get basename as string
        """
        return self.lineEditBasename.text()

    def setBasenameToolTip(self):
        """Set the tool tip for basename line edit based on the current mode."""
        self.lineEditBasename.setToolTip("Set basename for the layer.")

    def intervalChanges(self):
        """Slot when the text on the interval line edit changed.

        It set the text color to red, disable the calculate button,
        and update the calculate button tooltip.
        """
        try:
            interval = self.getInterval()
            if len(interval) == 0:
                raise Exception(self.tr("Interval can not be empty"))
            if len(interval) > 10:
                raise Exception(self.tr("Interval can not be more than 10"))
            self.lineEditIntervals.setStyleSheet("color: black;")
            self.btnCalculate.setEnabled(True)
            self.btnCalculate.setToolTip("")
        except Exception as e:
            pushMessage(str(e))
            self.lineEditIntervals.setStyleSheet("color: red;")
            self.btnCalculate.setEnabled(False)
            self.btnCalculate.setToolTip(self.tr("Please make sure the interval is correct."))

    def getInterval(self):
        """Get interval of as a list of integer or float based on the current mode.
        It also make sure that the list is ascending
        """
        intervalText = self.lineEditIntervals.text()
        # remove white space
        intervalText = "".join(intervalText.split())
        interval = intervalText.split(";")
        if (
            self.comboBoxReachabilityMode.currentText()
            == self.reachabilityMode["isochrone"]
        ):
            # try to convert to int for isochrone
            interval = [int(x) for x in interval if len(x) > 0]
        else:
            # try to convert to float for isodistance
            interval = [float(x) for x in interval if len(x) > 0]
        # sort it
        return sorted(interval)

    def setIntervalToolTip(self):
        """Set the tool tip for interval line edit based on the current mode."""
        if (self.comboBoxReachabilityMode.currentText()
                == self.reachabilityMode["isochrone"]):
            self.lineEditIntervals.setToolTip(
                self.tr('Set interval as integer in minutes, separated by ";" symbol')
            )
        else:
            self.lineEditIntervals.setToolTip(
                self.tr('Set interval as float in Km, separated by ";" symbol')
            )

    def getColorFromInterval(self):
        num_interval = len(self.getInterval())
        first_color = "00CC00"
        last_color = "CC0000"
        colors = {
            1: [first_color],
            2: [first_color, last_color],
            3: [first_color, 'CCCC00', last_color],
            4: [first_color, '88CC00', 'CC8800', last_color],
            5: [first_color, '66CC00', 'CCCC00', 'CC6600', last_color],
            6: [first_color, '51CC00', 'A3CC00', 'CCA300', 'CC5100', last_color],
            7: [first_color, '43CC00', '88CC00', 'CCCC00', 'CC8800', 'CC4300', last_color],
            8: [first_color, '3ACC00', '74CC00', 'AECC00', 'CCAE00', 'CC7400', 'CC3A00', last_color],
            9: [first_color, '33CC00', '66CC00', '99CC00', 'CCCC00', 'CC9900', 'CC6600', 'CC3300', last_color],
            10: [first_color, '2DCC00', '5ACC00', '88CC00', 'B5CC00', 'CCB500', 'CC8800', 'CC5A00', 'CC2D00',
                 last_color],
            11: [first_color, '28CC00', '51CC00', '7ACC00', 'A3CC00', 'CCCC00', 'CCA300', 'CC7A00', 'CC5100',
                 'CC2800', last_color],
            12: [first_color, '25CC00', '4ACC00', '6FCC00', '94CC00', 'B9CC00', 'CCB900', 'CC9400', 'CC6F00',
                 'CC4A00', 'CC2500', last_color]
        }
        if num_interval not in colors.keys():
            return []
        return colors[num_interval]
