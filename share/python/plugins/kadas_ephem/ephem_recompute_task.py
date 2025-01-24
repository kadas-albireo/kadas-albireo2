
import ephem
from enum import Enum
from datetime import datetime, timezone
import math

from qgis.PyQt.QtCore import QThread, QDateTime, QTimeZone

from qgis.core import QgsCoordinateReferenceSystem, QgsPoint

from kadas.kadascore import KadasCoordinateUtils
from kadas.kadasanalysis import KadasLineOfSight

class EphemComputeResult(object):
    def __init__(self):
        self.position = None
        self.celestialBody = EphemComputeTask.CelestialBody.SUN
        self.angle = 0.0
        self.riseValueText = ""
        self.setValueText = ""
        self.azimuthElevationValueText = ""
        self.azimuthVisible = False
        self.zenithValueText = ""
        self.moonPhase = 0.0


class EphemComputeTaskCanceled(Exception):
    pass

class EphemComputeTask(QThread):

    class TimezoneType(Enum):
        TIMEZONE_SYSTEM = 0
        TIMEZONE_UTC = 1
        TIMEZONE_LOCAL = 2

    class CelestialBody(Enum):
        SUN = 1
        MOON = 2

    def __init__(self, parent=None):
        QThread.__init__(self, parent)

        self.wgsPos = None
        self.celestialBody = self.CelestialBody.SUN
        self.considerRelief = False
        self.dateTime = QDateTime.currentDateTime()
        self.timezoneType = self.TimezoneType.TIMEZONE_SYSTEM

        self.result = EphemComputeResult()

    def compute(self, wgsPos, celestialBody, considerRelief, dateTime, timezoneType):
        self.wgsPos = wgsPos
        self.celestialBody = celestialBody
        self.considerRelief = considerRelief
        self.dateTime = dateTime
        self.timezoneType = timezoneType
        
        if self.isRunning():
            self.cancel()
            self.wait()

        self.canceled = False
        self.start()

    def getResult(self):
        return self.result
    
    def cancel(self):
        self.canceled = True


    def run(self):
        
        try:
            ts = self.getTimestamp(self.dateTime)
    
            self.result.position = self.wgsPos
            self.result.celestialBody = self.celestialBody

            home = ephem.Observer()
            home.lat = str(self.wgsPos.y())
            home.lon = str(self.wgsPos.x())
            home.date = ephem.Date(datetime.fromtimestamp(ts, timezone.utc))
    
            if self.celestialBody == self.CelestialBody.SUN:
                ## Sun ##
    
                sun = ephem.Sun()
                sun.compute(home)
                self.__check_canceled()
    
                self.result.azimuthElevationValueText = "%s %s" % (self.formatDMS(sun.az), self.formatDMS(sun.alt, True))
                self.result.angle = -self.azDec(sun.az)
    
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
                self.__check_canceled()

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
                if self.considerRelief:
                    tvisible = self.search_body_visible(ephem.Sun(), sunrise, sunset) if sunrise and sunset else None
                    if tvisible:
                        sunset = self.search_body_relief_crossing(ephem.Sun(), sunset, tvisible) if sunset else None
                        sunrise = self.search_body_relief_crossing(ephem.Sun(), sunrise, tvisible) if sunrise else None
                    else:
                        sunset = None
                        sunrise = None
    
                if sunrise and (not sunset or sunrise < sunset):
                    self.result.riseValueText = "%s" % self.timestampToHourString(sunrise)
                else:
                    self.result.riseValueText = "-"
    
                if sunset and (not sunrise or sunset > sunrise):
                    self.result.setValueText = "%s" % self.timestampToHourString(sunset)
                else:
                    self.result.setValueText = "-"
    
                self.result.zenithValueText = self.timestampToHourString(suntransit)
                self.result.azimuthVisible = sunrise is not None and sunset is not None and ts >= sunrise and ts <= sunset
    
            else:
                ## Moon ##
    
                moon = ephem.Moon()
                moon.compute(home)
                self.result.azimuthElevationValueText = "%s %s" % (self.formatDMS(moon.az), self.formatDMS(moon.alt, True))
                self.result.angle = -self.azDec(moon.az)
    
                # Compute moonrise and moonset taking relief into account
                try:
                    moonset = ephem.to_timezone(home.next_setting(moon), ephem.UTC).timestamp()
                except:
                    moonset = None
    
                moon.compute(home)
                self.__check_canceled()
                
                moontransit = ephem.to_timezone(home.next_transit(moon), ephem.UTC).timestamp()
                if moonset and moontransit > moonset:
                    moontransit = ephem.to_timezone(home.previous_transit(moon), ephem.UTC).timestamp()
    
                moon.compute(home)
                self.__check_canceled()

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
                if self.considerRelief:
                    tvisible = self.search_body_visible(ephem.Moon(), moonrise, moonset) if moonrise and moonset else None
                    if tvisible:
                        moonset = self.search_body_relief_crossing(ephem.Moon(), moonset, tvisible) if moonset else None
                        moonrise = self.search_body_relief_crossing(ephem.Moon(), moonrise, tvisible) if moonrise else None
                    else:
                        moonset = None
                        moonrise = None
    
                if moonrise and (not moonset or moonrise < moonset):
                    self.result.riseValueText = "%s" % self.timestampToHourString(moonrise)                
                else:
                    self.result.riseValueText = "-"
    
                if moonset and (not moonrise or moonset > moonrise):
                    self.result.setValueText = "%s" % self.timestampToHourString(moonset)
                else:
                    self.result.setValueText = "-"
    
                # Moon phase
                self.result.moonPhase = moon.phase
    
                self.result.azimuthVisible = moonrise is not None and moonset is not None and ts >= moonrise and ts <= moonset
        
        except EphemComputeTaskCanceled:
            return
        except Exception as exception:
            print("Ephem compute task error: " + str(exception))

    def timestampToHourString(self, timestamp):
        if self.timezoneType == self.TimezoneType.TIMEZONE_UTC:
            return QDateTime.fromSecsSinceEpoch(round(timestamp), QTimeZone.utc()).toString("hh:mm")
        elif self.timezoneType == self.TimezoneType.TIMEZONE_LOCAL:
            tz = QTimeZone(KadasCoordinateUtils.getTimezoneAtPos(self.wgsPos, QgsCoordinateReferenceSystem("EPSG:4326")))
            return QDateTime.fromSecsSinceEpoch(round(timestamp), tz).toString("hh:mm")
        else:
            return QDateTime.fromSecsSinceEpoch(round(timestamp)).toString("hh:mm")

    def getTimestamp(self, datetime):
        if self.timezoneType == EphemComputeTask.TimezoneType.TIMEZONE_UTC:
            datetime = QDateTime(datetime.date(), datetime.time(), QTimeZone.utc())
        elif self.timezoneType == EphemComputeTask.TimezoneType.TIMEZONE_LOCAL:
            tz = QTimeZone(KadasCoordinateUtils.getTimezoneAtPos(self.wgsPos, QgsCoordinateReferenceSystem("EPSG:4326")))
            datetime = QDateTime(datetime.date(), datetime.time(), QTimeZone(tz))
        return datetime.toSecsSinceEpoch()

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
            self.__check_canceled()

            newranges = []
            for entry in ranges:
                self.__check_canceled()

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
        self.__check_canceled()

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
        bodyVisibility = KadasLineOfSight.computeTargetVisibility(QgsPoint(self.mrcPos.x(), self.mrcPos.y(), 0), body_pos, QgsCoordinateReferenceSystem("EPSG:3857"), 10000, False, True)
        self.__check_canceled()

        return bodyVisibility
    
    def __check_canceled(self):
        if self.canceled:
            raise EphemComputeTaskCanceled()
