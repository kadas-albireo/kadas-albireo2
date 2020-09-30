import logging

from qgis.core import (
    QgsSettings,
    QgsGpsDetector,
    QgsCoordinateReferenceSystem,
    QgsCoordinateTransform,
    QgsGpsInformation,
    QgsProject
)

from qgis.utils import iface

from PyQt5.QtCore import QEventLoop, pyqtSignal, QObject

from kadasrouting.utilities import waitcursor

LOG = logging.getLogger(__name__)


@waitcursor
def getGpsConnection():
    gpsConnection = None
    port = str(QgsSettings().value("/kadas/gps_port", ""))
    gpsDetector = QgsGpsDetector(port)
    loop = QEventLoop()

    def gpsDetected(connection):
        gpsConnection = connection  # NOQA
        loop.exit()

    def gpsDetectionFailed():
        loop.exit()

    gpsDetector.detected.connect(gpsDetected)
    gpsDetector.detectionFailed.connect(gpsDetectionFailed)
    gpsDetector.advance()
    loop.exec_()

    return gpsConnection


def getMockupGpsConnection():
    return GpsConnection(iface.mapCanvas())


class GpsConnection(QObject):

    statusChanged = pyqtSignal(object)

    def __init__(self, canvas):
        QObject.__init__(self)
        self._canvas = canvas
        # Update the point when the canvas extent changed.
        self._canvas.extentsChanged.connect(self.currentLocationChanged)

    def currentGPSInformation(self):
        center = self._canvas.center()
        outCrs = QgsCoordinateReferenceSystem(4326)
        canvasCrs = self._canvas.mapSettings().destinationCrs()
        transform = QgsCoordinateTransform(canvasCrs, outCrs, QgsProject.instance())
        wgspoint = transform.transform(center)
        info = QgsGpsInformation()

        info.speed = 25
        info.latitude = wgspoint.y()
        info.longitude = wgspoint.x()
        info.direction = 45
        info.elevation = 200

        return info

    def currentLocationChanged(self):
        LOG.debug('Current GPS location is changed')
        self.statusChanged.emit(self.currentGPSInformation())
