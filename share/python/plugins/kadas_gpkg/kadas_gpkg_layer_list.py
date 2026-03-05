import os
import re

from qgis.core import QgsMapLayer, QgsProject
from qgis.PyQt.QtCore import Qt
from qgis.PyQt.QtGui import QIcon
from qgis.PyQt.QtWidgets import QListWidget, QListWidgetItem


class KadasGpkgLayersList(QListWidget):
    LayerIdRole = Qt.UserRole + 1
    LayerTypeRole = Qt.UserRole + 2
    LayerSizeRole = Qt.UserRole + 3
    LayerProviderNameRole = Qt.UserRole + 4
    WARN_SIZE = 50 * 1024 * 1024
    PROJECT_LAYER_PROVIDERS = ["bullseye", "guide_grid", "KadasItemLayer"]

    def __init__(self, parent):

        QListWidget.__init__(self, parent)

        supported_providers = [
            "delimitedtext",
            "gdal",
            "gpx",
            "mssql",
            "ogr",
            "postgres",
            "spatialite",
            "wcs",
            "wms",
            "WFS",
            "arcgisfeatureserver",
        ]
        supported_providers.extend(self.PROJECT_LAYER_PROVIDERS)

        layers = {}
        for layer in QgsProject.instance().mapLayers().values():
            provider_name = layer.dataProvider().name()
            if provider_name in supported_providers:
                layers[layer.id()] = layer

        for layerid in sorted(layers.keys()):
            layer = QgsProject.instance().mapLayer(layerid)
            if not layer:
                continue

            file_size = self.__get_layer_size(layer)

            item = QListWidgetItem(layer.name())
            item.setData(KadasGpkgLayersList.LayerIdRole, layerid)
            item.setData(KadasGpkgLayersList.LayerTypeRole, layer.type())
            item.setData(KadasGpkgLayersList.LayerSizeRole, file_size)
            item.setData(KadasGpkgLayersList.LayerProviderNameRole, layer.dataProvider().name())
            if file_size is not None and file_size < KadasGpkgLayersList.WARN_SIZE:
                item.setCheckState(Qt.Checked)
                item.setIcon(QIcon())
            else:
                item.setCheckState(Qt.Unchecked)
                item.setIcon(QIcon(":/images/themes/default/mIconWarning.svg"))

            self.addItem(item)

    def updateLayerList(self, existingOutputGpkg):
        # Update layer list
        for i in range(0, self.count()):
            item = self.item(i)
            layerid = item.data(KadasGpkgLayersList.LayerIdRole)
            layer = QgsProject.instance().mapLayer(layerid)
            # Disable layers already in GPKG
            gpkgLayer = existingOutputGpkg and (
                layer.source().startswith(existingOutputGpkg)
                or layer.source().startswith("GPKG:" + existingOutputGpkg)
            )
            if gpkgLayer:
                item.setFlags(item.flags() & ~(Qt.ItemIsSelectable | Qt.ItemIsEnabled))
                item.setIcon(QIcon(":/images/themes/default/mIconSuccess.svg"))
            else:
                size = item.data(KadasGpkgLayersList.LayerSizeRole)
                item.setFlags(item.flags() | Qt.ItemIsSelectable | Qt.ItemIsEnabled)
                if size is not None and int(size) < KadasGpkgLayersList.WARN_SIZE:
                    item.setIcon(QIcon())
                else:
                    item.setIcon(QIcon(":/images/themes/default/mIconWarning.svg"))

    def getSelectedLayers(self):
        layers = {}
        for i in range(0, self.count()):
            item = self.item(i)

            provider_name = item.data(KadasGpkgLayersList.LayerProviderNameRole)
            if provider_name in self.PROJECT_LAYER_PROVIDERS:
                continue

            if item.flags() & Qt.ItemIsEnabled and item.checkState() == Qt.Checked:
                layerid = item.data(KadasGpkgLayersList.LayerIdRole)
                layers[layerid] = item.data(KadasGpkgLayersList.LayerTypeRole)
        return layers

    def getUnselectedProjectLayers(self):
        layers = {}
        for i in range(0, self.count()):
            item = self.item(i)

            provider_name = item.data(KadasGpkgLayersList.LayerProviderNameRole)
            if provider_name not in self.PROJECT_LAYER_PROVIDERS:
                continue

            if item.checkState() != Qt.Checked:
                layerid = item.data(KadasGpkgLayersList.LayerIdRole)
                layer = QgsProject.instance().mapLayer(layerid)
                layers[layerid] = layer
        return layers

    def __get_layer_size(self, layer):

        # Size 0 for redlining layers
        if layer.dataProvider().name() in self.PROJECT_LAYER_PROVIDERS:
            return 0

        filename = layer.source()
        # Strip options from <filename>|options
        pos = filename.find("|")
        if pos >= 0:
            filename = filename[:pos]
        # Resolve file url
        if filename.startswith("file://"):
            filename = filename[7 : filename.find("?")]
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
                return None

        try:
            return os.path.getsize(filename)
        except Exception:
            return None
