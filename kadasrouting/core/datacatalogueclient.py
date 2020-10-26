import os
import json

from PyQt5.QtCore import (
    QUrl,
    QFile,
    QDir
)

from PyQt5.QtNetwork import (
    QNetworkRequest,
    QNetworkReply
)

from qgis.core import (
    QgsNetworkAccessManager
)

from kadasrouting.utilities import appDataDir, waitcursor

from pyplugin_installer import unzip


class DataCatalogueClient():

    NOT_INSTALLED, UPDATABLE, UP_TO_DATE = range(3)

    DEFAULT_URL = "https://geoinfo-kadas.op.intra2.admin.ch/portal/sharing/rest"

    def __init__(self, url=None):
        self.url = url or self.DEFAULT_URL

    def dataTimestamp(self, itemid):
        filename = os.path.join(self.folderForDataItem(itemid), "metadata")
        try:
            with open(filename) as f:
                timestamp = json.read(f)["modified"]
            return timestamp
        except Exception:
            return None

    def getAvailableTiles(self):
        url = f'{self.url}/search?q=tags:"valhalla_network"&f=pjson'
        response = QgsNetworkAccessManager.blockingGet(QNetworkRequest(QUrl(url)))
        if response.error() != QNetworkReply.NoError:
            raise Exception("Response error:" + response.error())
        responsejson = json.loads(response.content().data())
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
        return QFile(self.folderForDataItem(itemid)).removeRecursively()

    def folderForDataItem(self, itemid):
        return os.path.join(appDataDir(), "tiles", itemid)


dataCatalogueClient = DataCatalogueClient()
