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

from kadasrouting.utilities import appDataDir, waitcursor, pushWarning

from pyplugin_installer import unzip

class DataCatalogueClient():

    NOT_INSTALLED, UPDATABLE, UP_TO_DATE = range(3)

    DEFAULT_URL = "" #TODO

    def __init__(self, url=None):       
        self.url = url or self.DEFAULT_URL

    def dataTimestamp(self, itemid):
        filename = os.path.join(self.folderForDataItem(itemid), "timestamp")
        try:
            with open(filename) as f:
                timestamp = f.read()
            return timestamp
        except Exception:
            return None

    def getAvailableTiles(self):
        url = f'{self.url}/search?q=owner:"geosupport.fsta" tags:"valhalla"&f=pjson' 
        response = QgsNetworkAccessManager.blockingGet(QNetworkRequest(QUrl(url)))
        if response.error() != QNetworkReply.NoError:        
            raise Exception(response.error())
        responsejson = json.loads(response.content.decode())            
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
            tiles.append({
                    "id": itemid,
                    "name": result["name"],
                    "title": result["title"],
                    "timestamp": int(result["modified"]),
                    "status": status
                })
        return tiles

    def install(self, itemid, timestamp):
        if self._downloadAndUnzip(itemid):
            filename = os.path.join(self.folderForDataItem(itemid), "timestamp")            
            with open(filename, "w") as f:
                f.write(timestamp)            
            return True
        else:
            return False

    @waitcursor
    def _downloadAndUnzip(self, itemid):
        url = f'{self.url}/content/items/{itemid}/data'
        response = QgsNetworkAccessManager.blockingGet(QNetworkRequest(QUrl(url)))
        if response.error() == QNetworkReply.NoError:   
            request = reply.request()
            tmpDir = QDir.tempPath()
            filename = f"{itemid}.zip"
            tmpPath = QDir.cleanPath(os.path.join(tmpDir, filename))
            file = QFile(tmpPath)
            file.open(QFile.WriteOnly)
            file.write(reply.readAll())
            file.close()
            targetFolder = self.folderForDataItem(itemid)
            removed = QDir(targetFolder).removeRecursively()
            if not removed:
                return False
            unzip(tmpPath, targetFolder)              
            QFile(tmpPath).remove()
            return True
        else:
            return False

    def uninstall(self, itemid):                
        return QFile(self.folderForDataItem(itemid)).removeRecursively()

    def folderForDataItem(self, itemid):
        return os.path.join(appDataDir(), "tiles", itemid)
            
dataCatalogueClient = DataCatalogueClient()
