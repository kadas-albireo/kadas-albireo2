import os
import json
import logging

from pyplugin_installer import unzip

from PyQt5.QtCore import QUrl, QFile, QDir
from PyQt5.QtNetwork import QNetworkRequest, QNetworkReply

from qgis.core import QgsNetworkAccessManager

from kadasrouting.utilities import appDataDir, waitcursor

LOG = logging.getLogger(__name__)

# Obtained from Valhalla installer
DEFAULT_DATA_TILES_PATH = r'C:/Program Files/KadasAlbireo/share/kadas/routing/default'


class DataCatalogueClient():

    NOT_INSTALLED, UPDATABLE, UP_TO_DATE = range(3)

    # DEFAULT_URL = "https://geoinfo-kadas.op.intra2.admin.ch/portal/sharing/rest"

    DEFAULT_URL = 'https://ch-milgeo.maps.arcgis.com/sharing/rest'

    def __init__(self, url=None):
        self.url = url or self.DEFAULT_URL

    def dataTimestamp(self, itemid):
        filename = os.path.join(self.folderForDataItem(itemid), "metadata")
        try:
            with open(filename) as f:
                timestamp = json.load(f)["modified"]
            LOG.debug('timestamp is %s' % timestamp)
            return timestamp
        except Exception as e:
            LOG.debug('metadata file is failed to read: %s' % e)
            return None

    def getAvailableTiles(self):
        # https://ch-milgeo.maps.arcgis.com/sharing/rest/
        # search?q=owner:%22geosupport.fsta%22%20tags:%22valhalla%22&f=pjson
        url = f'{self.url}/search?q=owner:%22geosupport.fsta%22%20tags:%22valhalla%22&f=pjson'
        response = QgsNetworkAccessManager.blockingGet(QNetworkRequest(QUrl(url)))
        if response.error() != QNetworkReply.NoError:
            raise Exception("Response error:" + response.error())
        responsejson = json.loads(response.content().data())
        LOG.debug('response from data repository: %s' % responsejson)
        tiles = []
        for result in responsejson["results"]:
            itemid = result["id"]
            timestamp = self.dataTimestamp(itemid)
            if timestamp is None:
                status = self.NOT_INSTALLED
            elif timestamp < result["modified"]:
                status = self.UPDATABLE
            else:
                status = self.UP_TO_DATE
            tile = dict(result)
            tile["status"] = status
            tiles.append(tile)
        return tiles

    def install(self, data):
        itemid = data["id"]
        if self._downloadAndUnzip(itemid):
            filename = os.path.join(self.folderForDataItem(itemid), "metadata")
            LOG.debug('install data on %s' % filename)
            with open(filename, "w") as f:
                json.dump(data, f)
            return True
        else:
            return False

    @waitcursor
    def _downloadAndUnzip(self, itemid):
        url = f'{self.url}/content/items/{itemid}/data'
        response = QgsNetworkAccessManager.blockingGet(QNetworkRequest(QUrl(url)))
        if response.error() == QNetworkReply.NoError:
            tmpDir = QDir.tempPath()
            filename = f"{itemid}.zip"
            tmpPath = QDir.cleanPath(os.path.join(tmpDir, filename))
            file = QFile(tmpPath)
            file.open(QFile.WriteOnly)
            file.write(response.content().data())
            file.close()
            targetFolder = self.folderForDataItem(itemid)
            removed = QDir(targetFolder).removeRecursively()
            if not removed:
                return False
            unzip.unzip(tmpPath, targetFolder)
            QFile(tmpPath).remove()
            return True
        else:
            return False

    def uninstall(self, itemid):
        path = self.folderForDataItem(itemid)
        LOG.debug('uninstall/remove from %s' % path)
        return QDir(self.folderForDataItem(itemid)).removeRecursively()

    def folderForDataItem(self, itemid):
        if itemid == 'default':
            return DEFAULT_DATA_TILES_PATH
        return os.path.join(appDataDir(), "tiles", itemid)


dataCatalogueClient = DataCatalogueClient()
