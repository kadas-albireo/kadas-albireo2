from qgis.PyQt.QtCore import Qt
from qgis.PyQt.QtGui import QIcon
from qgis.PyQt.QtWidgets import QListWidget, QListWidgetItem
from qgis.core import QgsProject, QgsMapLayer

import os
import re

class KadasGpkgLayersList(QListWidget):

    LayerIdRole = Qt.ItemDataRole.UserRole + 1
    LayerTypeRole = Qt.ItemDataRole.UserRole + 2
    LayerSizeRole = Qt.ItemDataRole.UserRole + 3
    WARN_SIZE = 50 * 1024 * 1024

    def __init__(self, parent):

        QListWidget.__init__(self, parent)

        self.layers = {}
        local_providers = ["delimitedtext", "gdal", "gpx", "mssql", "ogr", "postgres", "spatialite", "wcs", "wms", "WFS", "arcgisfeatureserver"]

        for layer in QgsProject.instance().mapLayers().values():
            provider = "unknown"
            if layer.type() == QgsMapLayer.VectorLayer or layer.type() == QgsMapLayer.RasterLayer:
                provider = layer.dataProvider().name()
            elif layer.type() == QgsMapLayer.PluginLayer:
                provider = "plugin"

            if provider in local_providers:
                self.layers[layer.id()] = layer.type()

        for layerid in sorted(self.layers.keys()):
            layer = QgsProject.instance().mapLayer(layerid)
            if not layer:
                continue
            filename = layer.source()
            # Strip options from <filename>|options
            pos = filename.find("|")
            if pos >= 0:
                filename = filename[:pos]
            # Resolve file url
            if filename.startswith("file://"):
                filename = filename[7:filename.find("?")]
            # Remove vsi prefix
            if filename.startswith("/vsi"):
                filename = re.sub(r"/vsi\w+/", "", filename)
                while not os.path.isfile(filename):
                    newfilename = os.path.dirname(filename)
                    if newfilename == filename:
                        filename = None
                        break
                    filename = newfilename
                if not filename:
                    continue

            try:
                filesize = os.path.getsize(filename)
            except:
                filesize = None
            item = QListWidgetItem(layer.name())
            item.setData(KadasGpkgLayersList.LayerIdRole, layerid)
            item.setData(KadasGpkgLayersList.LayerTypeRole, self.layers[layerid])
            item.setData(KadasGpkgLayersList.LayerSizeRole, filesize)
            if filesize is not None and filesize < KadasGpkgLayersList.WARN_SIZE:
                item.setCheckState(Qt.CheckState.Checked)
                item.setIcon(QIcon())
            else:
                item.setCheckState(Qt.CheckState.Unchecked)
                item.setIcon(QIcon(":/images/themes/default/mIconWarning.svg"))

            self.addItem(item)

    def updateLayerList(self, existingOutputGpkg):
        # Update layer list
        for i in range(0, self.count()):
            item = self.item(i)
            layerid = item.data(KadasGpkgLayersList.LayerIdRole)
            layer = QgsProject.instance().mapLayer(layerid)
            # Disable layers already in GPKG
            gpkgLayer = existingOutputGpkg and (layer.source().startswith(existingOutputGpkg) or layer.source().startswith("GPKG:" + existingOutputGpkg))
            if gpkgLayer:
                item.setFlags(item.flags() & ~(Qt.ItemFlag.ItemIsSelectable | Qt.ItemFlag.ItemIsEnabled))
                item.setIcon(QIcon(":/images/themes/default/mIconSuccess.svg"))
            else:
                size = item.data(KadasGpkgLayersList.LayerSizeRole)
                item.setFlags(item.flags() | Qt.ItemFlag.ItemIsSelectable | Qt.ItemFlag.ItemIsEnabled)
                if size is not None and int(size) < KadasGpkgLayersList.WARN_SIZE:
                    item.setIcon(QIcon())
                else:
                    item.setIcon(QIcon(":/images/themes/default/mIconWarning.svg"))

    def getSelectedLayers(self):
        layers = {}
        for i in range(0, self.count()):
            item = self.item(i)
            if item.flags() & Qt.ItemFlag.ItemIsEnabled and item.checkState() == Qt.CheckState.Checked:
                layerid = item.data(KadasGpkgLayersList.LayerIdRole)
                layers[layerid] = item.data(KadasGpkgLayersList.LayerTypeRole)
        return layers
