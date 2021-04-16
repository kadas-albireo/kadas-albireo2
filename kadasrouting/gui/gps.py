import logging

from qgis.core import QgsApplication

from kadasrouting.utilities import waitcursor

LOG = logging.getLogger(__name__)


@waitcursor
def getGpsConnection():
    gpsConnectionList = QgsApplication.gpsConnectionRegistry().connectionList()
    LOG.debug('gpsConnectionList = {}'.format(gpsConnectionList))
    if len(gpsConnectionList) > 0:
        return gpsConnectionList[0]
    else:
        return None
