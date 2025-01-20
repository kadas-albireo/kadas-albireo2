
from datetime import datetime, timezone
import ephem
import math
import os

from qgis.PyQt.QtCore import *
from qgis.PyQt.QtGui import *
from qgis.PyQt.QtWidgets import *
from qgis.PyQt.uic import loadUiType
from qgis.core import *
from qgis.gui import *
from kadas.kadascore import *
from kadas.kadasgui import *
from kadas.kadasanalysis import *


Ui_EphemToolWidget, _ = loadUiType(
    os.path.join(os.path.dirname(__file__), "EphemToolWidget.ui")
)

class EphemToolWidget(KadasBottomBar):

    close = pyqtSignal()

    TIMEZONE_SYSTEM = 0
    TIMEZONE_UTC = 1
    TIMEZONE_LOCAL = 2

    def __init__(self, iface):
        KadasBottomBar.__init__(self, iface.mapCanvas())

        self.iface = iface

        self.ui = Ui_EphemToolWidget()
        self.ui.setupUi(self)

        self.ui.closeButton.setSizePolicy(QSizePolicy.Preferred, QSizePolicy.Preferred)
        self.ui.closeButton.setIcon(QIcon(":/kadas/icons/close"))
        self.ui.closeButton.setToolTip(self.tr("Close"))
        self.ui.closeButton.clicked.connect(self.close)

        self.ui.dateTimeEdit.setDateTime(QDateTime.currentDateTime())
        self.ui.dateTimeEdit.editingFinished.connect(self.recompute)
        self.ui.timezoneCombo.addItem(self.tr("System time"), EphemToolWidget.TIMEZONE_SYSTEM)
        self.ui.timezoneCombo.addItem(self.tr("UTC"), EphemToolWidget.TIMEZONE_UTC)
        self.ui.timezoneCombo.addItem(self.tr("Local time at position"), EphemToolWidget.TIMEZONE_LOCAL)
        self.ui.timezoneCombo.currentIndexChanged.connect(self.recompute)
        self.ui.checkBoxRelief.toggled.connect(self.recompute)
        self.ui.tabWidgetOutput.setEnabled(False)
        self.ui.tabWidgetOutput.currentChanged.connect(self.recompute)

        self.busyOverlay = QLabel(self.tr("Calculating..."))
        self.busyOverlay.setStyleSheet("QLabel { background-color: white;}")
        self.busyOverlay.setAlignment(Qt.AlignHCenter|Qt.AlignVCenter)
        self.busyOverlay.setVisible(False)
        self.busyOverlay.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        self.ui.verticalLayout.addWidget(self.busyOverlay)

        self.wgsPos = None
        self.mrcPos = None

        self.sunAzIcon = KadasSymbolItem(QgsCoordinateReferenceSystem("EPSG:4326"))
        self.sunAzIcon.setup( os.path.join(os.path.dirname(__file__), "icons/az_sun.svg"), 0.5, 1.0 )
        self.sunAzIcon.setVisible(False)
        KadasMapCanvasItemManager.addItem(self.sunAzIcon)

        self.moonAzIcon = KadasSymbolItem(QgsCoordinateReferenceSystem("EPSG:4326"))
        self.moonAzIcon.setup( os.path.join(os.path.dirname(__file__), "icons/az_moon.svg"), 0.5, 1.0 )
        KadasMapCanvasItemManager.addItem(self.moonAzIcon)
        self.moonAzIcon.setVisible(False)

    def getTimestamp(self):
        datetime = self.ui.dateTimeEdit.dateTime()
        if self.ui.timezoneCombo.currentData() == EphemToolWidget.TIMEZONE_UTC:
            datetime = QDateTime(datetime.date(), datetime.time(), QTimeZone.utc())
        elif self.ui.timezoneCombo.currentData() == EphemToolWidget.TIMEZONE_LOCAL:
            tz = QTimeZone(KadasCoordinateUtils.getTimezoneAtPos(self.wgsPos, QgsCoordinateReferenceSystem("EPSG:4326")))
            datetime = QDateTime(datetime.date(), datetime.time(), QTimeZone(tz))
        return datetime.toSecsSinceEpoch()

    def setPos(self, wgsPos, mrcPos):
        self.wgsPos = wgsPos
        self.mrcPos = mrcPos

    def cleanup(self):
        KadasMapCanvasItemManager.removeItem(self.sunAzIcon)
        self.sunAzIcon = None
        KadasMapCanvasItemManager.removeItem(self.moonAzIcon)
        self.moonAzIcon = None

    def recompute(self):
        self.sunAzIcon.setVisible(False)
        self.moonAzIcon.setVisible(False)

        if not self.wgsPos:
            return

        QApplication.setOverrideCursor(Qt.WaitCursor)

        if self.ui.checkBoxRelief.isChecked():
            self.busyOverlay.setVisible(True)
            self.ui.tabWidgetOutput.setEnabled(False)
            QApplication.instance().processEvents(QEventLoop.ExcludeUserInputEvents)

        self.ui.tabWidgetOutput.setEnabled(True)
        font = self.ui.labelPositionValue.font()
        font.setItalic(False)
        self.ui.labelPositionValue.setFont(font)
        x_str = QgsCoordinateFormatter.formatX(self.wgsPos.x(), QgsCoordinateFormatter.FormatDegreesMinutesSeconds, 1)
        y_str = QgsCoordinateFormatter.formatY(self.wgsPos.y(), QgsCoordinateFormatter.FormatDegreesMinutesSeconds, 1)
        self.ui.labelPositionValue.setText(y_str + " " + x_str)
        tz = KadasCoordinateUtils.getTimezoneAtPos(self.wgsPos, QgsCoordinateReferenceSystem("EPSG:4326"))
        self.ui.labelTimezone.setText(tz.data().decode('utf-8'))

        ts = self.getTimestamp()

        home = ephem.Observer()
        home.lat = str(self.wgsPos.y())
        home.lon = str(self.wgsPos.x())
        home.date = ephem.Date(datetime.fromtimestamp(ts, timezone.utc))

        if self.ui.tabWidgetOutput.currentIndex() == 0:
            ## Sun ##

            sun = ephem.Sun()
            sun.compute(home)
            self.ui.labelAzimuthElevationValue.setText("%s %s" % (self.formatDMS(sun.az), self.formatDMS(sun.alt, True)))
            self.sunAzIcon.setPosition(KadasItemPos.fromPoint(self.wgsPos))
            self.sunAzIcon.setAngle(-self.azDec(sun.az))

            # Compute sunrise and sunset taking relief into account
            try:
                sunset = ephem.to_timezone(home.next_setting(sun), ephem.UTC).timestamp()
            except:
                sunset = None

            sun.compute(home)
            suntransit = ephem.to_timezone(home.next_transit(sun), ephem.UTC).timestamp()
            if sunset and suntransit > sunset:
                suntransit = ephem.to_timezone(home.previous_transit(sun), ephem.UTC).timestamp()

            sun.compute(home)
            if sun.alt >= 0:
                try:
                    sunrise = ephem.to_timezone(home.previous_rising(sun), ephem.UTC).timestamp()
                except:
                    sunrise = None
            else:
                try:
                    sunrise = ephem.to_timezone(home.next_rising(sun), ephem.UTC).timestamp()
                except:
                    sunrise = None
            if self.ui.checkBoxRelief.isChecked():
                tvisible = self.search_body_visible(ephem.Sun(), sunrise, sunset) if sunrise and sunset else None
                if tvisible:
                    sunset = self.search_body_relief_crossing(ephem.Sun(), sunset, tvisible) if sunset else None
                    sunrise = self.search_body_relief_crossing(ephem.Sun(), sunrise, tvisible) if sunrise else None
                else:
                    sunset = None
                    sunrise = None

            if sunrise and (not sunset or sunrise < sunset):
                self.ui.labelSunRiseValue.setText("%s" % self.timestampToHourString(sunrise))
            else:
                self.ui.labelSunRiseValue.setText("-")
            if sunset and (not sunrise or sunset > sunrise):
                self.ui.labelSunSetValue.setText("%s" % self.timestampToHourString(sunset))
            else:
                self.ui.labelSunSetValue.setText("-")
            self.ui.labelZenithValue.setText(self.timestampToHourString(suntransit))

            self.sunAzIcon.setVisible(sunrise is not None and sunset is not None and ts >= sunrise and ts <= sunset)
            self.moonAzIcon.setVisible(False)

        else:
            ## Moon ##

            moon = ephem.Moon()
            moon.compute(home)
            self.ui.labelMoonAzimuthElevationValue.setText("%s %s" % (self.formatDMS(moon.az), self.formatDMS(moon.alt, True)))
            self.moonAzIcon.setPosition(KadasItemPos.fromPoint(self.wgsPos))
            self.moonAzIcon.setAngle(-self.azDec(moon.az))

            # Compute moonrise and moonset taking relief into account
            try:
                moonset = ephem.to_timezone(home.next_setting(moon), ephem.UTC).timestamp()
            except:
                moonset = None

            moon.compute(home)
            moontransit = ephem.to_timezone(home.next_transit(moon), ephem.UTC).timestamp()
            if moonset and moontransit > moonset:
                moontransit = ephem.to_timezone(home.previous_transit(moon), ephem.UTC).timestamp()

            moon.compute(home)
            if moon.alt >= 0:
                try:
                    moonrise = ephem.to_timezone(home.previous_rising(moon), ephem.UTC).timestamp()
                except:
                    moonrise = None
            else:
                try:
                    moonrise = ephem.to_timezone(home.next_rising(moon), ephem.UTC).timestamp()
                except:
                    moonrise = None
            if self.ui.checkBoxRelief.isChecked():
                tvisible = self.search_body_visible(ephem.Moon(), moonrise, moonset) if moonrise and moonset else None
                if tvisible:
                    moonset = self.search_body_relief_crossing(ephem.Moon(), moonset, tvisible) if moonset else None
                    moonrise = self.search_body_relief_crossing(ephem.Moon(), moonrise, tvisible) if moonrise else None
                else:
                    moonset = None
                    moonrise = None

            if moonrise and (not moonset or moonrise < moonset):
                self.ui.labelMoonRiseValue.setText("%s" % self.timestampToHourString(moonrise))
            else:
                self.ui.labelMoonRiseValue.setText("-")
            if moonset and (not moonrise or moonset > moonrise):
                self.ui.labelMoonSetValue.setText("%s" % self.timestampToHourString(moonset))
            else:
                self.ui.labelMoonSetValue.setText("-")

            # Moon phase
            moon_image_suffix = math.floor(round(moon.phase/12.5)*12.5)
            self.ui.labelMoonPhaseIcon.setPixmap(QPixmap(os.path.join(os.path.dirname(__file__), f"icons/moon_{moon_image_suffix}.svg")))
            self.ui.labelMoonPhaseValue.setText("%.2f%%" % moon.phase)

            self.sunAzIcon.setVisible(False)

            self.moonAzIcon.setVisible(moonrise is not None and moonset is not None and ts >= moonrise and ts <= moonset)

        self.busyOverlay.setVisible(False)
        self.ui.tabWidgetOutput.setEnabled(True)
        QApplication.restoreOverrideCursor()

    def timestampToHourString(self, timestamp):
        if self.ui.timezoneCombo.currentData() == EphemToolWidget.TIMEZONE_UTC:
            return QDateTime.fromSecsSinceEpoch(round(timestamp), QTimeZone.utc()).toString("hh:mm")
        elif self.ui.timezoneCombo.currentData() == EphemToolWidget.TIMEZONE_LOCAL:
            tz = QTimeZone(KadasCoordinateUtils.getTimezoneAtPos(self.wgsPos, QgsCoordinateReferenceSystem("EPSG:4326")))
            return QDateTime.fromSecsSinceEpoch(round(timestamp), tz).toString("hh:mm")
        else:
            return QDateTime.fromSecsSinceEpoch(round(timestamp)).toString("hh:mm")

    def formatDMS(self, val, sign=False):
        strval = str(val)
        if sign and strval[0] != "-":
            strval = "+" + strval
        parts = strval.split(":")
        if len(parts) == 3:
            return parts[0] + "°" + parts[1] + "'" + parts[2] + "\""
        else:
            return strval

    def azDec(self, val):
        strval = str(val)
        parts = strval.split(":")
        if len(parts) == 3:
            return float(parts[0]) + float(parts[1]) / 60 + float(parts[2]) / 3600
        else:
            return 0

    def search_body_visible(self, body, tmin, tmax):
        # Tree like search to find one any time when body is visible
        ranges = [(tmin, tmax)]
        # Search up to 10min precision
        maxsteps = math.ceil(1 + math.log2(round((tmax - tmin) / 600)))
        for i in range(0, maxsteps):
            newranges = []
            for entry in ranges:
                mid = 0.5 * (entry[0] + entry[1])
                if self.body_is_visible(self.compute_body_position(mid, body)):
                    return mid
                else:
                    newranges.append((entry[0], mid))
                    newranges.append((mid, entry[1]))
            ranges = newranges
        return None

    def search_body_relief_crossing(self, body, spheretime, zenithtime):
        # Binary search up to 1min precision
        mid = 0.5 * (spheretime + zenithtime)
        if abs(spheretime - zenithtime) < 60:
            return mid
        if self.body_is_visible(self.compute_body_position(mid, body)):
            return self.search_body_relief_crossing(body, spheretime, mid)
        else:
            return self.search_body_relief_crossing(body, mid, zenithtime)

    def compute_body_position(self, timestamp, body):
        home = ephem.Observer()
        home.lat = str(self.wgsPos.y())
        home.lon = str(self.wgsPos.x())
        home.date = ephem.Date(datetime.fromtimestamp(timestamp, timezone.utc))
        body.compute(home)
        azimuth_rad = float(body.az)
        alt_rad = float(body.alt)
        # Assume body 100km distant
        r = 100000
        body_x = self.mrcPos.x() + r * math.sin(azimuth_rad) * math.cos(alt_rad)
        body_y = self.mrcPos.y() + r * math.cos(azimuth_rad) * math.cos(alt_rad)
        body_z = r * math.sin(alt_rad)
        return QgsPoint(body_x, body_y, body_z)

    def body_is_visible(self, body_pos):
        # 100m resolution
        return KadasLineOfSight.computeTargetVisibility(QgsPoint(self.mrcPos.x(), self.mrcPos.y(), 0), body_pos, QgsCoordinateReferenceSystem("EPSG:3857"), 10000, False, True)
