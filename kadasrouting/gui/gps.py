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

@waitcursor
def getGpsConnection():
    gpsConnection = None
    port = str(QgsSettings().value("/kadas/gps_port", ""))
    gpsDetector = QgsGpsDetector(port)
    loop = QEventLoop()

    def gpsDetected(connection):
        gpsConnection = connection
        loop.exit()

    def gpsDetectionFailed():
        loop.exit()

    gpsDetector.detected.connect(gpsDetected)
    gpsDetector.detectionFailed.connect(gpsDetectionFailed)
    gpsDetector.advance()
    loop.exec_()
    
    return gpsConnection

def getMockupGpsConnection():
    return GpsConnection()

class GpsConnection(QObject):

    statusChanged = pyqtSignal(object)

    def currentGPSInformation(self):
        center = iface.mapCanvas().center()
        outCrs = QgsCoordinateReferenceSystem(4326)
        canvasCrs = iface.mapCanvas().mapSettings().destinationCrs()
        transform = QgsCoordinateTransform(canvasCrs, outCrs, QgsProject.instance())
        wgspoint = transform.transform(center)
        info = QgsGpsInformation()

        info.speed = 25
        info.latitude = wgspoint.y()
        info.longitude = wgspoint.x()
        info.direction = 45
        info.elevation = 200

        return info


