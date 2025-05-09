# -*- coding: utf-8 -*-

from qgis.PyQt.QtCore import Qt, QTemporaryDir, QEventLoop
from qgis.PyQt.QtWidgets import QDialog, QProgressDialog, QMessageBox, QApplication

from qgis.core import QgsProject, QgsPathResolver, QgsMapLayer, Qgis, QgsCoordinateTransform, QgsPluginLayer
from qgis.gui import *

from kadas.kadasgui import KadasMapRect, KadasItemLayer, KadasItemLayerRegistry

import os
import json
import mimetypes
import sqlite3
import shutil
import uuid
from lxml import etree as ET

from .kadas_gpkg_export_dialog import KadasGpkgExportDialog
from .kadas_gpkg_export_base import KadasGpkgExportBase

class KadasGpkgExport(KadasGpkgExportBase):

    PROPERTY_ITEM_TO_BE_REMOVED = "flagToBeRemoved"

    def __init__(self, iface):
        KadasGpkgExportBase.__init__(self)
        self.iface = iface

        self.kadasGpkgExportDialog = None

    def run(self):
        # Check dialog already open
        if self.kadasGpkgExportDialog is not None:
            return

        self.kadasGpkgExportDialog = KadasGpkgExportDialog(self.iface.mainWindow(), self.iface)
        self.kadasGpkgExportDialog.finished.connect(self.__dialogFinished)
        self.kadasGpkgExportDialog.show()

    def __dialogFinished(self, result):
        if result == QDialog.Accepted:
            self.__export()

        self.kadasGpkgExportDialog.finished.disconnect()
        self.kadasGpkgExportDialog.clear()
        self.kadasGpkgExportDialog = None

    def __export(self):
        # Write project to temporary file
        tmpdir = QTemporaryDir()

        gpkg_filename = self.kadasGpkgExportDialog.outputFile()
        gpkg_writefile = gpkg_filename

        if self.kadasGpkgExportDialog.clearOutputFile():
            gpkg_writefile = tmpdir.filePath(os.path.basename(gpkg_filename))

        # Open database
        try:
            conn = sqlite3.connect(gpkg_writefile)
        except:
            QMessageBox.warning(self.iface.mainWindow(), self.tr("Error"), self.tr("Unable to create or open output file"))
            return

        pdialog = QProgressDialog(
            self.tr("Writing %s...") % os.path.basename(gpkg_filename),
            self.tr("Cancel"), 0, 0,  self.iface.mainWindow())
        pdialog.setWindowModality(Qt.WindowModal)
        pdialog.setWindowTitle(self.tr("GPKG Export"))
        pdialog.show()

        cursor = conn.cursor()
        self.init_gpkg(cursor)
        self.init_gpkg_qgis(cursor)
        conn.commit()
        conn.close()
        QApplication.processEvents(QEventLoop.ExcludeUserInputEvents)

        # Collect layer sources
        layer_sources = set()
        for layerId, layer in QgsProject.instance().mapLayers().items():
            layer_sources.add(layer.source())
            if layer.providerType() == "ogr":
                # QgsVectorLayer::encodedSource separates the path from the subset query and only passes the path to the path resolver
                layer_sources.add(layer.source().split("|")[0])
        layer_sources = list(layer_sources)

        # Copy all selected local layers to the database
        added_layer_ids = []
        added_layers_by_source = {}
        messages = []
        if not self.write_layers(self.kadasGpkgExportDialog.selectedLayers(), gpkg_writefile, pdialog, added_layer_ids, added_layers_by_source, messages, self.kadasGpkgExportDialog.buildPyramids(), self.kadasGpkgExportDialog.filterExtent(), self.kadasGpkgExportDialog.filterExtentCrs(), self.kadasGpkgExportDialog.rasterExportScale()):
            pdialog.hide()
            QMessageBox.warning(self.iface.mainWindow(), self.tr("GPKG Export"), self.tr("The operation was canceled."))
            return

        project = QgsProject.instance()
        prev_filename = project.fileName()
        prev_dirty = project.isDirty()
        tmpfile = tmpdir.filePath("gpkg_project.qgs")
        project.setFileName(tmpfile)

        # Flag redlining items outside export extent
        if self.kadasGpkgExportDialog.filterExtent() is not None:
            self.__flagRedliningItemsOutsideExtent(self.kadasGpkgExportDialog.filterExtent(), self.kadasGpkgExportDialog.filterExtentCrs())

        additional_resources = {}
        preprocessorId = QgsPathResolver.setPathWriter(lambda path: self.rewriteProjectPaths(path, gpkg_filename, added_layers_by_source, layer_sources, additional_resources))
        project.write()
        QgsPathResolver.removePathWriter(preprocessorId)

        # Restore project state

        # Remove Flag redlining items outside export extent
        if self.kadasGpkgExportDialog.filterExtent() is not None:
            self.__removeRedliningItemsFlag()

        project.setFileName(prev_filename if prev_filename else None)
        project.setDirty(prev_dirty)

        # Parse project and replace data sources if necessary
        parser = ET.XMLParser(strip_cdata=False)
        doc = ET.parse(tmpfile, parser=parser)
        if not doc:
            QMessageBox.warning(self.iface.mainWindow(), self.tr("Error"), self.tr("Invalid project"))
            return

        # Replace layer provider types in project file
        sources = []
        for projectlayerEl in doc.find("projectlayers"):
            layerId = projectlayerEl.find("id").text
            if layerId in added_layer_ids:
                layer = QgsProject.instance().mapLayer(layerId)
                if layer.type() == QgsMapLayer.VectorLayer:
                    projectlayerEl.find("provider").text = "ogr"
                elif layer.type() == QgsMapLayer.RasterLayer:
                    projectlayerEl.find("provider").text = "gdal"

        # Remove redlining items outside export extent from project file
        if self.kadasGpkgExportDialog.filterExtent() is not None:
            self.__removeFlaggedRedlining(doc)

        # Add additional resources
        conn = sqlite3.connect(gpkg_writefile)
        cursor = conn.cursor()
        for path, resource_id in additional_resources.items():
            self.add_resource(cursor, path, resource_id)

        # Write project file to GPKG
        project_xml = ET.tostring(doc.getroot())
        self.write_project(cursor, project_xml)

        conn.commit()
        conn.close()

        if self.kadasGpkgExportDialog.clearOutputFile():
            try:
                os.remove(gpkg_filename)
            except:
                pass
            try:
                shutil.move(gpkg_writefile, gpkg_filename)
            except:
                QMessageBox.warning(self.iface.mainWindow(), self.tr("Error"), self.tr("Unable to create output file"))
                return

        pdialog.hide()
        self.iface.messageBar().pushMessage(
            self.tr("GPKG export completed"), "", Qgis.Info, 5)

        if messages:
            QMessageBox.warning(
                self.iface.mainWindow(),
                self.tr("GPKG Export"),
                self.tr("The following layers were not exported to the GeoPackage:\n- %s") % "\n- ".join(messages))

    def init_gpkg_qgis(self, cursor):
        # Create extension
        cursor.execute(
            'CREATE TABLE IF NOT EXISTS gpkg_extensions (table_name TEXT,column_name TEXT,extension_name TEXT NOT NULL,definition TEXT NOT NULL,scope TEXT NOT NULL,CONSTRAINT ge_tce UNIQUE (table_name, column_name, extension_name))')
        extension_record = (None, None, 'qgis',
                            'http://github.com/pka/qgpkg/blob/master/'
                            'qgis_geopackage_extension.md',
                            'read-write')
        cursor.execute('SELECT count(1) FROM gpkg_extensions WHERE extension_name=?', (extension_record[2],))
        if cursor.fetchone()[0] == 0:
            cursor.execute('INSERT INTO gpkg_extensions VALUES (?,?,?,?,?)', extension_record)

        # Create qgis_projects table
        cursor.execute('CREATE TABLE IF NOT EXISTS qgis_projects (name TEXT PRIMARY KEY, xml TEXT NOT NULL)')

        # Create qgis_resources table
        cursor.execute('CREATE TABLE IF NOT EXISTS qgis_resources (name TEXT PRIMARY KEY, mime_type TEXT NOT NULL, content BLOB NOT NULL)')

    def write_project(self, cursor, project_xml):
        """ Write or update qgis project """
        project_name = "qgpkg"
        cursor.execute('SELECT count(1) FROM qgis_projects WHERE name=?', (project_name,))
        if cursor.fetchone()[0] == 0:
            cursor.execute('INSERT INTO qgis_projects VALUES (?,?)', (project_name, project_xml))
        else:
            cursor.execute('UPDATE qgis_projects SET xml=? WHERE name=?', (project_xml, project_name))

    def rewriteProjectPaths(self, path, gpkg_filename, added_layers_by_source, layer_sources, additional_resources):
        if not path:
            return path

        if path in added_layers_by_source:
            # Datasource newly added to GPKG: rewrite as GPKG path
            layer = QgsProject.instance().mapLayer(added_layers_by_source[path])
            if layer.type() == QgsMapLayer.VectorLayer:
                return "@gpkg_file@|layername=" + self.safe_name(layer.name())
            elif layer.type() == QgsMapLayer.RasterLayer:
                return "GPKG:@gpkg_file@:" + self.safe_name(layer.name())
        elif path and (path.startswith(gpkg_filename) or path.startswith("GPKG:" + gpkg_filename)):
            # Previous GPKG sources: replace GPKG path with placeholder
            return path.replace(gpkg_filename, "@gpkg_file@")
        elif os.path.isfile(path) and not path in layer_sources:
            # Other resource: Add it to resources,
            if not path in additional_resources:
                additional_resources[path] = str(uuid.uuid1()) + os.path.splitext(path)[1]

            return "@qgis_resources@/%s" % additional_resources[path]
        
        for added_layer_source in added_layers_by_source.keys():
            if path in added_layer_source:
                # Datasource newly added to GPKG: rewrite as GPKG path
                layer = QgsProject.instance().mapLayer(added_layers_by_source[added_layer_source])
                if layer.type() == QgsMapLayer.VectorLayer:
                    return "@gpkg_file@"
                elif layer.type() == QgsMapLayer.RasterLayer:
                    return "GPKG:@gpkg_file@:" + self.safe_name(layer.name())

        # No action
        return path

    def add_resource(self, cursor, path, resource_id):
        """ Add a resource file to qgis_resources """
        with open(path, 'rb') as fh:
            blob = fh.read()
            mime_type = mimetypes.MimeTypes().guess_type(path)[0]
            cursor.execute('SELECT count(1) FROM qgis_resources WHERE name=?', (resource_id,))
            if cursor.fetchone()[0] == 0:
                cursor.execute('INSERT INTO qgis_resources VALUES(?, ?, ?)', (resource_id, mime_type, sqlite3.Binary(blob)))
            else:
                cursor.execute('UPDATE qgis_resources SET mime_type=?, content=? WHERE name=?', (mime_type, sqlite3.Binary(blob), resource_id))

    def __flagRedliningItemsOutsideExtent(self, extent, crs):
        """ Flag redlining items outside export extent """

        rectExportExtent = KadasMapRect(extent.xMinimum(), extent.yMinimum(), extent.xMaximum(), extent.yMaximum())
        mapSettings = self.iface.mapCanvas().mapSettings()

        for layer in KadasItemLayerRegistry.getItemLayers():
            for item in layer.items().values():
                if not item.intersects(rectExportExtent, mapSettings):
                    # Flag redlining item
                    item.setProperty(self.PROPERTY_ITEM_TO_BE_REMOVED, True)

    def __removeRedliningItemsFlag(self):
        """ Remove redlining items flag """
        
        for layer in KadasItemLayerRegistry.getItemLayers():
            for item in layer.items().values():
                if self.PROPERTY_ITEM_TO_BE_REMOVED in item.dynamicPropertyNames():
                    # Un-Flag redlining item
                    item.setProperty(self.PROPERTY_ITEM_TO_BE_REMOVED, None)

    def __removeFlaggedRedlining(self, doc):
        """ Remove redlining items outside export extent """
        
        for mapItemEl in doc.iterfind("projectlayers/maplayer/MapItem"):
            
            nameAttribute = mapItemEl.attrib.get("name", str())
            if not nameAttribute.startswith("Kadas"):
                return

            cdata = json.loads(mapItemEl.text)

            props = cdata.get("props", None)
            if props is None:
                continue

            flagToBeRemoved = props.get(self.PROPERTY_ITEM_TO_BE_REMOVED, None)
            if flagToBeRemoved is True:
                # Remove redlining item
                mapItemEl.getparent().remove(mapItemEl)
                continue
            
