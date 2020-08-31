from qgis.core import QgsSettings, QgsGpsDetector

from PyQt5.QtCore import QEventLoop

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