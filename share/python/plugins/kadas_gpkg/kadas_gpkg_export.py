# -*- coding: utf-8 -*-

from qgis.PyQt.QtCore import *
from qgis.PyQt.QtGui import *
from qgis.PyQt.QtWidgets import *

from qgis.core import *
from qgis.gui import *

import os
import re
import mimetypes
import sqlite3
import shutil
import uuid
from lxml import etree as ET

from .kadas_gpkg_export_base import KadasGpkgExportBase
from .ui_kadas_gpkg_export_dialog import Ui_KadasGpkgExportDialog


class KadasGpkgExportDialog(QDialog):

    def __init__(self, parent, iface):
        QDialog.__init__(self, parent)
        self.ui = Ui_KadasGpkgExportDialog()
        self.ui.setupUi(self)
        self.ui.buttonBox.button(QDialogButtonBox.Ok).setEnabled(False)

        self.ui.spinBoxExportScale.setValue(int(iface.mapCanvas().mapSettings().scale()))

        self.ui.buttonSelectFile.clicked.connect(self.__selectOutputFile)
        self.ui.checkBoxClear.toggled.connect(self.__updateLocalLayerList)
        self.ui.checkBoxExportScale.toggled.connect(self.ui.spinBoxExportScale.setEnabled)

    def __selectOutputFile(self):
        lastDir = QSettings().value("/UI/lastImportExportDir", ".")
        filename = QFileDialog.getSaveFileName(self, self.tr("Select GPKG File..."), lastDir, self.tr("GPKG Database (*.gpkg)"), "", QFileDialog.DontConfirmOverwrite)[0]

        if not filename:
            return

        if not filename.lower().endswith(".gpkg"):
            filename += ".gpkg"

        QSettings().setValue("/UI/lastImportExportDir", os.path.dirname(filename))
        self.ui.lineEditOutputFile.setText(filename)

        self.ui.buttonBox.button(QDialogButtonBox.Ok).setEnabled(filename is not None)
        self.__updateLocalLayerList()

    def __updateLocalLayerList(self):
        self.ui.listWidgetLayers.updateLayerList(self.ui.lineEditOutputFile.text() if not self.ui.checkBoxClear.isChecked() else None)

    def outputFile(self):
        return self.ui.lineEditOutputFile.text()

    def clearOutputFile(self):
        return self.ui.checkBoxClear.isChecked()

    def selectedLayers(self):
        return self.ui.listWidgetLayers.getSelectedLayers()

    def buildPyramids(self):
        return self.ui.checkBoxPyramids.isChecked()

    def rasterExportScale(self):
        return self.ui.spinBoxExportScale.value() if self.ui.checkBoxExportScale.isChecked() else None


class KadasGpkgExport(KadasGpkgExportBase):

    def __init__(self, iface):
        KadasGpkgExportBase.__init__(self)
        self.iface = iface

    def run(self):

        dialog = KadasGpkgExportDialog(self.iface.mainWindow(), self.iface)
        if dialog.exec_() != QDialog.Accepted:
            return

        # Write project to temporary file
        tmpdir = QTemporaryDir()

        gpkg_filename = dialog.outputFile()
        selected_layers = dialog.selectedLayers()
        gpkg_writefile = gpkg_filename

        if dialog.clearOutputFile():
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
        if not self.write_layers(selected_layers, gpkg_writefile, pdialog, added_layer_ids, added_layers_by_source, messages, dialog.buildPyramids(), None, None, dialog.rasterExportScale()):
            pdialog.hide()
            QMessageBox.warning(self.iface.mainWindow(), self.tr("GPKG Export"), self.tr("The operation was canceled."))
            return

        project = QgsProject.instance()
        prev_filename = project.fileName()
        prev_dirty = project.isDirty()
        tmpfile = tmpdir.filePath("gpkg_project.qgs")
        project.setFileName(tmpfile)
        additional_resources = {}
        preprocessorId = QgsPathResolver.setPathWriter(lambda path: self.rewriteProjectPaths(path, gpkg_filename, added_layers_by_source, layer_sources, additional_resources))
        project.write()
        QgsPathResolver.removePathWriter(preprocessorId)
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

        if dialog.clearOutputFile():
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
        else:
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
