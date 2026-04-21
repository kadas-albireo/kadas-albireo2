import os
import sqlite3

from qgis.PyQt import uic
from qgis.PyQt.QtCore import QSettings, Qt
from qgis.PyQt.QtWidgets import (
    QDialog,
    QDialogButtonBox,
    QFileDialog,
    QListWidgetItem,
)
from qgis.PyQt.QtXml import QDomDocument

PLUGIN_DIR = os.path.dirname(__file__)

Ui_KadasGpkgImportDialog, _ = uic.loadUiType(
    os.path.join(PLUGIN_DIR, "kadas_gpkg_import_dialog.ui")
)


class KadasGpkgImportDialog(QDialog):
    def __init__(self, parent, iface, filename=None):
        QDialog.__init__(self, parent)
        self.iface = iface
        self.ui = Ui_KadasGpkgImportDialog()
        self.ui.setupUi(self)
        self.ui.labelDataWarning.setVisible(False)
        self.ui.widgetProjectImport.setVisible(False)
        self.ui.buttonBox.button(QDialogButtonBox.StandardButton.Ok).setEnabled(False)
        self.ui.buttonSelectFile.clicked.connect(self.__selectInputFile)
        self.ui.radioButtonImportLayers.toggled.connect(self.ui.listWidgetLayers.setEnabled)

        self.layerIdRole = Qt.ItemDataRole.UserRole + 1
        self.xml = None

        if filename is not None:
            self.__selectInputFile(filename)
        else:
            self.ui.buttonSelectFile.click()

    def __selectInputFile(self, filename):
        if not filename:
            lastDir = QSettings().value("/UI/lastImportExportDir", ".")
            filename = QFileDialog.getOpenFileName(
                self, self.tr("Select GPKG File..."), lastDir, self.tr("GPKG Database (*.gpkg)")
            )[0]

            if not filename:
                return

            if not filename.lower().endswith(".gpkg"):
                filename += ".gpkg"

            QSettings().setValue("/UI/lastImportExportDir", os.path.dirname(filename))

        # Read project and extract layers
        self.xml = self.__read_gpkg_project(filename)

        if self.xml is None:
            self.ui.labelDataWarning.setVisible(True)
            self.ui.widgetProjectImport.setVisible(False)
        else:
            self.ui.labelDataWarning.setVisible(False)
            self.ui.widgetProjectImport.setVisible(True)

        self.ui.lineEditInputFile.setText(filename)
        self.ui.buttonBox.button(QDialogButtonBox.StandardButton.Ok).setEnabled(True)

    def __read_gpkg_project(self, gpkg_filename):
        self.ui.listWidgetLayers.clear()

        try:
            conn = sqlite3.connect(gpkg_filename)
        except Exception:
            return None

        """ Read qgis project """
        project_name = "qgpkg"
        cursor = conn.cursor()
        try:
            cursor.execute("SELECT xml FROM qgis_projects WHERE name=?", (project_name,))
        except Exception:
            return None
        qgis_projects = cursor.fetchone()
        if qgis_projects is None:
            conn.close()
            return None
        xml = qgis_projects[0]
        doc = QDomDocument()
        doc.setContent(xml)
        maplayers = doc.elementsByTagName("maplayer")

        for i in range(0, maplayers.size()):
            maplayer = maplayers.at(i)
            try:
                layerid = maplayer.firstChildElement("id").text()
                layername = maplayer.firstChildElement("layername").text()
            except Exception:
                # Need at least layerid and layername
                continue
            item = QListWidgetItem(layername)
            item.setCheckState(Qt.CheckState.Unchecked)
            item.setData(self.layerIdRole, layerid)
            self.ui.listWidgetLayers.addItem(item)

        conn.close()
        return xml

    def gpkgFilename(self):
        return self.ui.lineEditInputFile.text()

    def projectXml(self):
        return self.xml

    def selectedLayersOnly(self):
        return self.ui.radioButtonImportLayers.isChecked()

    def selectedLayerIds(self):
        layerIds = []
        for i in range(0, self.ui.listWidgetLayers.count()):
            item = self.ui.listWidgetLayers.item(i)
            if item.checkState() == Qt.CheckState.Checked:
                layerIds.append(item.data(self.layerIdRole))
        return layerIds
