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

mockedResponse= {
  "query": "tags:\"valhalla_network\"",
  "total": 3,
  "start": 1,
  "num": 10,
  "nextStart": -1,
  "results": [
    {
      "id": "13d2b773e2b84ded916d11946ffb0c89",
      "owner": "UE1644793@IFC1",
      "created": 1603049721475,
      "isOrgItem": True,
      "modified": 1603049805298,
      "guid": None,
      "name": "monaco_tiles.zip",
      "title": "Monaco Routing Network",
      "type": "Code Sample",
      "typeKeywords": [
        "C",
        "Code",
        "Sample"
      ],
      "description": None,
      "tags": ["valhalla_network"],
      "snippet": None,
      "thumbnail": None,
      "documentation": None,
      "extent": [],
      "categories": [],
      "spatialReference": None,
      "accessInformation": None,
      "licenseInfo": None,
      "culture": "fr-ch",
      "properties": None,
      "url": None,
      "proxyFilter": None,
      "access": "public",
      "size": -1,
      "appCategories": [],
      "industries": [],
      "languages": [],
      "largeThumbnail": None,
      "banner": None,
      "screenshots": [],
      "listed": False,
      "numComments": 0,
      "numRatings": 0,
      "avgRating": 0,
      "numViews": 0,
      "scoreCompleteness": 16,
      "groupDesignations": None
    },
    {
      "id": "5fcbbff3fc064932a0a09dbfce162db7",
      "owner": "UE1644793@IFC1",
      "created": 1603049474719,
      "isOrgItem": True,
      "modified": 1603049890819,
      "guid": None,
      "name": "andorra_tiles.zip",
      "title": "Andorra Routing Network",
      "type": "Code Sample",
      "typeKeywords": [
        "Code",
        "Python",
        "Sample"
      ],
      "description": "Example routing network for Valhalla Engine.",
      "tags": ["valhalla_network"],
      "snippet": None,
      "thumbnail": None,
      "documentation": None,
      "extent": [],
      "categories": [],
      "spatialReference": None,
      "accessInformation": None,
      "licenseInfo": None,
      "culture": "fr-ch",
      "properties": None,
      "url": None,
      "proxyFilter": None,
      "access": "public",
      "size": -1,
      "appCategories": [],
      "industries": [],
      "languages": [],
      "largeThumbnail": None,
      "banner": None,
      "screenshots": [],
      "listed": False,
      "numComments": 0,
      "numRatings": 0,
      "avgRating": 0,
      "numViews": 0,
      "scoreCompleteness": 21,
      "groupDesignations": None
    },
    {
      "id": "ad8a2f8f41f249008a472de36113cd91",
      "owner": "UE1644793@IFC1",
      "created": 1603049576709,
      "isOrgItem": True,
      "modified": 1603049826946,
      "guid": None,
      "name": "liechtenstein_tiles.zip",
      "title": "Liechtenstein Routing Network",
      "type": "Code Sample",
      "typeKeywords": [
        "C",
        "Code",
        "Sample"
      ],
      "description": "Example routing network for Valhalla engine.",
      "tags": ["valhalla_network"],
      "snippet": None,
      "thumbnail": None,
      "documentation": None,
      "extent": [],
      "categories": [],
      "spatialReference": None,
      "accessInformation": None,
      "licenseInfo": None,
      "culture": "fr-ch",
      "properties": None,
      "url": None,
      "proxyFilter": None,
      "access": "public",
      "size": -1,
      "appCategories": [],
      "industries": [],
      "languages": [],
      "largeThumbnail": None,
      "banner": None,
      "screenshots": [],
      "listed": False,
      "numComments": 0,
      "numRatings": 0,
      "avgRating": 0,
      "numViews": 0,
      "scoreCompleteness": 21,
      "groupDesignations": None
    }
  ]
}

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
            responsejson = mockedResponse
            #raise Exception(response.error())
        else:
            responsejson = json.loads(response.content.decode())            
        tiles = []
        for i, result in enumerate(responsejson["results"]):
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
                    "status": i#status
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
