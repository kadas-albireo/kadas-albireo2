# -*- coding: utf-8 -*-

import os
import sqlite3
import tempfile

from kadas.kadasgui import KadasMapCanvasItemManager, KadasMapItem, KadasProjectMigration
from qgis.core import (
    Qgis,
    QgsApplication,
    QgsMapLayer,
    QgsMapLayerFactory,
    QgsMapLayerType,
    QgsMeshLayer,
    QgsPathResolver,
    QgsProject,
    QgsRasterLayer,
    QgsReadWriteContext,
    QgsVectorLayer,
    QgsVectorTileLayer,
)
from qgis.PyQt.QtCore import QFileInfo, QObject
from qgis.PyQt.QtWidgets import (
    QDialog,
    QMessageBox,
)
from qgis.PyQt.QtXml import QDomDocument, QDomNodeList

from .kadas_gpkg_import_dialog import KadasGpkgImportDialog


class KadasGpkgImport(QObject):

    def __init__(self, iface):
        QObject.__init__(self)
        self.iface = iface

    def run(self, gpkg_filename=None):

        importDialog = KadasGpkgImportDialog(self.iface.mainWindow(), self.iface, gpkg_filename)
        if importDialog.exec() != QDialog.DialogCode.Accepted:
            return

        gpkg_filename = importDialog.gpkgFilename()
        xml = importDialog.projectXml()
        if not xml:
            self.iface.addVectorLayerQuiet(
                gpkg_filename, QFileInfo(gpkg_filename).baseName(), "ogr"
            )
            self.iface.addRasterLayerQuiet(
                gpkg_filename, QFileInfo(gpkg_filename).baseName(), "gdal"
            )

        elif importDialog.selectedLayersOnly():

            try:
                conn = sqlite3.connect(importDialog.gpkgFilename())
            except Exception:
                return
            cursor = conn.cursor()

            layerIds = importDialog.selectedLayerIds()
            doc = QDomDocument()
            doc.setContent(xml)

            # Migrate 1.x projects if necessary
            filesToAttach = []
            KadasProjectMigration.migrateProjectXml("", doc, filesToAttach)

            maplayers = doc.elementsByTagName("maplayer")
            mapcanvasitems = None
            try:
                mapcanvasitems = (
                    doc.elementsByTagName("MapCanvasItems")
                    .at(0)
                    .toElement()
                    .elementsByTagName("MapItem")
                )
            except Exception:
                mapcanvasitems = QDomNodeList()

            extracted_resources = {}
            preprocessorId = QgsPathResolver.setPathPreprocessor(
                lambda path: self.gpkgResourceToAttachment(
                    path, cursor, gpkg_filename, extracted_resources
                )
            )

            context = QgsReadWriteContext()
            context.setPathResolver(QgsProject.instance().pathResolver())
            context.setProjectTranslator(QgsProject.instance())
            context.setTransformContext(QgsProject.instance().transformContext())

            failed = []
            addedLayers = []
            for i in range(0, maplayers.size()):
                maplayer = maplayers.at(i)
                try:
                    layerid = maplayer.firstChildElement("id").text()
                    layername = maplayer.firstChildElement("layername").text()
                except Exception:
                    # Don't process layers without id
                    continue
                if layerid not in layerIds:
                    continue
                if not self.addProjectLayer(maplayer.toElement(), context):
                    failed.append(layername)
                else:
                    addedLayers.append(layerid)

            for i in range(0, mapcanvasitems.size()):
                mapcanvasitem = mapcanvasitems.at(i).toElement()
                if mapcanvasitem.attribute("associatedLayer") in addedLayers:
                    item = KadasMapItem.fromXml(mapcanvasitem)
                    if item:
                        KadasMapCanvasItemManager.addItem(item)

            QgsPathResolver.removePathPreprocessor(preprocessorId)
            conn.close()

            if failed:
                QMessageBox.warning(
                    self.iface.mainWindow(),
                    self.tr("Import Errors"),
                    self.tr("The following layers could not be imported:%s")
                    % ("\n - " + "\n - ".join(failed)),
                )

        else:
            if QgsProject.instance().isDirty():
                ret = QMessageBox.question(
                    self.iface.mainWindow(),
                    self.tr("Save project?"),
                    self.tr(
                        "The project has unsaved changes. Do you want to save them before proceeding?"
                    ),
                    QMessageBox.StandardButton.Yes
                    | QMessageBox.StandardButton.No
                    | QMessageBox.StandardButton.Cancel,
                    QMessageBox.StandardButton.Cancel,
                )
                if ret == QMessageBox.StandardButton.Cancel:
                    return
                elif ret == QMessageBox.StandardButton.Yes and not self.iface.saveProject():
                    return

            try:
                conn = sqlite3.connect(importDialog.gpkgFilename())
            except Exception:
                return
            cursor = conn.cursor()

            # Create temporary folder
            tmpdir = tempfile.mkdtemp()

            # Write project to temporary dir
            self.iface.newProject(False)

            output = os.path.join(tmpdir, "gpkg_project.qgs")
            with open(output, "wb") as fh:
                if isinstance(xml, str):
                    fh.write(xml.encode("utf-8"))
                else:
                    fh.write(xml)

            # Read project, adjust paths and extract resources as necessary
            extracted_resources = {}
            preprocessorId = QgsPathResolver.setPathPreprocessor(
                lambda path: self.readProjectPaths(
                    path, cursor, gpkg_filename, tmpdir, extracted_resources
                )
            )

            self.iface.addProject(output)

            QgsProject.instance().setFileName(None)
            QgsProject.instance().setDirty(True)

            QgsPathResolver.removePathPreprocessor(preprocessorId)
            conn.close()

        self.iface.messageBar().pushMessage(self.tr("GPKG import completed"), "", Qgis.Info, 5)

    def read_project(self, cursor):
        """Read qgis project"""
        project_name = "qgpkg"
        try:
            cursor.execute("SELECT xml FROM qgis_projects WHERE name=?", (project_name,))
        except Exception:
            return None
        qgis_projects = cursor.fetchone()
        if qgis_projects is None:
            return None
        else:
            return qgis_projects[0]

    def readProjectPaths(self, path, cursor, gpkg_filename, tmpdir, extracted_resources):
        if not path:
            return path
        path = path.replace("@gpkg_file@", gpkg_filename)
        if path.startswith("@qgis_resources@"):
            resource_id = path.replace("@qgis_resources@/", "")
            if resource_id in extracted_resources:
                path = extracted_resources[resource_id]
            else:
                path = self.extract_resource(
                    cursor, os.path.join(tmpdir, resource_id), resource_id
                )
                extracted_resources[resource_id] = path
        return path

    def gpkgResourceToAttachment(self, path, cursor, gpkg_filename, extracted_resources):
        if not path:
            return path
        path = path.replace("@gpkg_file@", gpkg_filename)
        if path.startswith("@qgis_resources@"):
            resource_id = path.replace("@qgis_resources@/", "")
            if resource_id in extracted_resources:
                path = extracted_resources[resource_id]
            else:
                output = QgsProject.instance().createAttachedFile(resource_id)
                path = self.extract_resource(cursor, output, resource_id)
                extracted_resources[resource_id] = path
        return path

    def extract_resource(self, cursor, output, resource_id):
        """Extract a resource file from qgis_resources"""
        try:
            cursor.execute("SELECT content FROM qgis_resources WHERE name=?", (resource_id,))
        except Exception:
            return None
        with open(output, "wb") as fh:
            fh.write(cursor.fetchone()[0])
        return output

    def addProjectLayer(self, maplayerEl, context):
        layerType, ok = QgsMapLayerFactory.typeFromString(maplayerEl.attribute("type"))
        if not ok:
            # Invalid layer type
            return False

        mapLayer = None
        if layerType == QgsMapLayerType.VectorLayer:
            mapLayer = QgsVectorLayer()
        elif layerType == QgsMapLayerType.RasterLayer:
            mapLayer = QgsRasterLayer()
        elif layerType == QgsMapLayerType.MeshLayer:
            mapLayer = QgsMeshLayer()
        elif layerType == QgsMapLayerType.VectorTileLayer:
            mapLayer = QgsVectorTileLayer()
        elif layerType == QgsMapLayerType.PluginLayer:
            typeName = maplayerEl.attribute("name")
            mapLayer = QgsApplication.pluginLayerRegistry().createLayer(typeName)

        if mapLayer is None:
            return False

        mapLayer.readLayerXml(maplayerEl, context, QgsMapLayer.ReadFlags())
        if not mapLayer.isValid():
            return False
        QgsProject.instance().addMapLayer(mapLayer)
        return True
