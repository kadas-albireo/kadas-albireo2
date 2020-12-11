import os
import json
import logging

from pyplugin_installer import unzip

from PyQt5.QtCore import QUrl, QFile, QDir, QUrlQuery
from PyQt5.QtNetwork import QNetworkRequest, QNetworkReply

from qgis.core import QgsNetworkAccessManager

from kadasrouting.utilities import appDataDir, waitcursor, pushWarning

LOG = logging.getLogger(__name__)

DEFAULT_REPOSITORY_URLS = [
    'https://ch-milgeo.maps.arcgis.com/sharing/rest',
    'https://geoinfo-kadas.op.intra2.admin.ch/portal/sharing/rest'
]

DEFAULT_ACTIVE_REPOSITORY_URL = DEFAULT_REPOSITORY_URLS[0]


class DataCatalogueClient():

    # Status of data tiles
    NOT_INSTALLED = 0  # Available on remote repository, but not in local file system
    UPDATABLE = 1  # There is a local copy, but the one in remote repository is newer
    UP_TO_DATE = 2  # There is a local copy and is up to date compared to the one in remote repository
    LOCAL_ONLY = 3  # The data is only available locally
    LOCAL_DELETED = 4  # The local only data is deleted

    def __init__(self, url=None):
        self.url = url or DEFAULT_ACTIVE_REPOSITORY_URL

    @staticmethod
    def dataTimestamp(itemid):
        filename = os.path.join(DataCatalogueClient.folderForDataItem(itemid), "metadata")
        try:
            with open(filename) as f:
                timestamp = json.load(f)["modified"]
            LOG.debug('timestamp is %s' % timestamp)
            return timestamp
        except Exception as e:
            LOG.debug('metadata file is failed to read: %s' % e)
            return None

    def getTiles(self):
        try:
            remote_tiles = self.getRemoteTiles()
        except Exception as e:
            pushWarning('Cannot get tiles from the URL because %s ' % str(e))
            remote_tiles = []
        local_tiles = self.getLocalTiles()
        # Merge the tiles
        all_tiles = []
        all_tiles.extend(remote_tiles)
        remote_tile_ids = [remote_tile['id'] for remote_tile in remote_tiles]
        for local_tile in local_tiles:
            if local_tile['id'] not in remote_tile_ids:
                all_tiles.append(local_tile)

        return all_tiles

    def getRemoteTiles(self):
        query = QUrlQuery()
        url = QUrl(f'{self.url}/search')
        query.addQueryItem('q', 'owner:%22geosupport.fsta%22%20tags:%22valhalla%22')
        query.addQueryItem('f', 'pjson')
        url.setQuery(query.query())
        response = QgsNetworkAccessManager.blockingGet(QNetworkRequest(QUrl(url)))
        if response.error() != QNetworkReply.NoError:
            raise Exception(response.errorString())
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

    @staticmethod
    def getLocalTiles():
        folder_data = DataCatalogueClient.folderData()
        data_dirs = [f.path for f in os.scandir(folder_data) if f.is_dir()]
        LOG.debug(data_dirs)
        local_tiles = []
        for data_dir in data_dirs:
            LOG.debug(type(data_dir))
            try:
                metadata_file = os.path.join(data_dir, "metadata")
                LOG.debug(metadata_file)
                with open(metadata_file) as f:
                    tile = json.load(f)
                    tile['status'] = DataCatalogueClient.LOCAL_ONLY
                local_tiles.append(tile)
            except Exception as e:
                LOG.debug(e)
        return local_tiles

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
            targetFolder = DataCatalogueClient.folderForDataItem(itemid)
            removed = QDir(targetFolder).removeRecursively()
            if not removed:
                return False
            unzip.unzip(tmpPath, targetFolder)
            QFile(tmpPath).remove()
            return True
        else:
            return False

    @staticmethod
    def uninstall(itemid):
        path = DataCatalogueClient.folderForDataItem(itemid)
        LOG.debug('uninstall/remove from %s' % path)
        return QDir(DataCatalogueClient.folderForDataItem(itemid)).removeRecursively()

    @staticmethod
    def folderForDataItem(itemid):
        return os.path.join(DataCatalogueClient.folderData(), itemid)

    @staticmethod
    def folderData():
        return os.path.join(appDataDir(), "tiles")
