# -*- coding: utf-8 -*-
#
#    This program is free software; you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation; either version 2 of the License, or
#    (at your option) any later version.
#
#    copyright            : (C) 2015 by Sourcepole AG

from .ui.ui_printlayoutmanager import Ui_PrintLayoutManager

from qgis.PyQt.QtCore import *
from qgis.PyQt.QtGui import *
from qgis.PyQt.QtWidgets import *
from qgis.PyQt.QtXml import *

from qgis.core import *


class PrintLayoutManager(QDialog, Ui_PrintLayoutManager):

    def __init__(self, iface, parent=None):
        QDialog.__init__(self, parent)
        self.iface = iface
        self.layoutManager = QgsProject.instance().layoutManager()
        self.setupUi(self)

        self.__reloadPrintLayouts()

        QgsProject.instance().layoutManager().layoutAdded.connect(
            lambda view: self.__reloadPrintLayouts())
        QgsProject.instance().layoutManager().layoutRemoved.connect(
            self.__reloadPrintLayouts)
        self.listWidgetLayouts.selectionModel().selectionChanged.connect(
            self.__listSelectionChanged)
        self.pushButtonImport.clicked.connect(self.__import)
        self.pushButtonExport.clicked.connect(self.__export)
        self.pushButtonRemove.clicked.connect(self.__remove)

    def __reloadPrintLayouts(self, removedLayout=None):
        self.listWidgetLayouts.clear()
        # Loaded layouts
        for layout in self.layoutManager.printLayouts():
            if layout != removedLayout:
                name = layout.name()
                item = QListWidgetItem(name)
                item.setData(Qt.UserRole, layout)
                self.listWidgetLayouts.addItem(item)
        # Attached, unloaded layouts
        for path in QgsProject.instance().attachedFiles():
            if path.endswith(".qpt"):
                file = QFile(path)
                if file.open(QIODevice.ReadOnly):
                    reader = QXmlStreamReader(file)
                    reader.readNextStartElement()
                    name = reader.attributes().value("name")
                    item = QListWidgetItem(name)
                    item.setData(Qt.UserRole, path)
                    self.listWidgetLayouts.addItem(item)

        self.listWidgetLayouts.sortItems()

    def __listSelectionChanged(self):
        selected = len(self.listWidgetLayouts.selectedItems()) > 0
        fixedMode = False
        if selected:
            fixedMode = self.listWidgetLayouts.selectedItems()[0].text() == "Custom"
        self.pushButtonExport.setEnabled(selected)
        self.pushButtonRemove.setEnabled(selected and not fixedMode)

    def __import(self):
        lastDir = QSettings().value("/UI/lastImportExportDir", ".")
        filename = QFileDialog.getOpenFileName(
            self, self.tr("Import Layout"), lastDir,
            self.tr("QPT Files (*.qpt);;"))
        if type(filename) == tuple:
            filename = filename[0]
        if not filename:
            return
        QSettings().setValue(
            "/UI/lastImportExportDir", QFileInfo(filename).absolutePath())
        file = QFile(filename)
        if not file.open(QIODevice.ReadOnly):
            QMessageBox.critical(
                self, self.tr("Import Failed"),
                self.tr("Failed to open the input file for reading."),
                QMessageBox.Ok)
        else:
            doc = QDomDocument()
            doc.setContent(file)
            layoutEls = doc.elementsByTagName("Layout")
            if len(layoutEls) == 0:
                QMessageBox.critical(
                    self, self.tr("Import Failed"),
                    self.tr("The file does not appear to be a valid print layout."), QMessageBox.Ok)
                return
            layoutEl = layoutEls.at(0).toElement()

            layout = QgsPrintLayout(QgsProject.instance())
            layout.setName(layoutEl.attribute("name"))

            if not layout.loadFromTemplate(doc, QgsReadWriteContext()):
                QMessageBox.critical(self, self.tr("Import Failed"), self.tr(
                    "The file does not appear to be a valid print layout."),
                    QMessageBox.Ok)
                return

            self.layoutManager.addLayout(layout)

    def __export(self):
        lastDir = QSettings().value("/UI/lastImportExportDir", ".")
        filename = QFileDialog.getSaveFileName(
            self, self.tr("Export Layout"),
            lastDir, self.tr("QPT Files (*.qpt);;"))
        if type(filename) == tuple:
            filename = filename[0]
        if not filename:
            return
        QSettings().setValue(
            "/UI/lastImportExportDir", QFileInfo(filename).absolutePath())

        item = self.listWidgetLayouts.selectedItems()[0]
        layout = item.data(Qt.UserRole)
        if isinstance(layout, str):
            success = QFile(QgsProject.instance().attachedFile(layout)).copy(filename)
        else:
            layout.__class__ = QgsPrintLayout
            ctx = QgsReadWriteContext()
            ctx.setPathResolver(QgsProject.instance().pathResolver())
            success = layout.saveAsTemplate(filename, ctx)
        if not success:
            QMessageBox.critical(
                self, self.tr("Export Failed"),
                self.tr("Failed to open the output file for writing."),
                QMessageBox.Ok)

    def __remove(self):
        item = self.listWidgetLayouts.selectedItems()[0]
        layout = item.data(Qt.UserRole)
        if isinstance(layout, str):
            QgsProject.instance().removeAttachedFile(layout)
            self.__reloadPrintLayouts()
        else:
            layout.__class__ = QgsPrintLayout
            self.layoutManager.removeLayout(layout)
