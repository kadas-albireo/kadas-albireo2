# -*- coding: utf-8 -*-
#
#    This program is free software; you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation; either version 2 of the License, or
#    (at your option) any later version.
#
#    copyright            : (C) 2015 by Sourcepole AG

from qgis.PyQt.QtCore import *
from qgis.PyQt.QtGui import *
from qgis.PyQt.QtWidgets import *
from qgis.PyQt.QtXml import *
from qgis.core import *
from qgis.gui import *

from .ui.ui_cartouchedialog import Ui_CartoucheDialog


class CartoucheDialog(QDialog, Ui_CartoucheDialog):
    def __init__(self, scene, parent=None):
        QDialog.__init__(self, parent)
        self.setupUi(self)
        self.scene = scene
        self.mapcartoucheView.setInteractive(False)

        self.classification1.addItem(self.tr("RESTRICTED"), "Internal")
        self.classification1.addItem(self.tr("CONFIDENTIAL"), "Confidential")
        self.classification1.addItem(self.tr("SECRET"), "Secret")
        self.classification1.setCurrentIndex(-1)

        self.classification2.addItem(self.tr("RESTRICTED"), "Internal")
        self.classification2.addItem(self.tr("CONFIDENTIAL"), "Confidential")
        self.classification2.addItem(self.tr("SECRET"), "Secret")
        self.classification2.setCurrentIndex(-1)

        self.exercisedateLE.dateChanged.connect(self.updatePrintLayout)
        self.classification1.currentIndexChanged.connect(self.updatePrintLayout)
        self.classification1.lineEdit().setPlaceholderText(self.tr("CLASSIFICATION"))
        self.classification2.currentIndexChanged.connect(self.updatePrintLayout)
        self.classification2.lineEdit().setPlaceholderText(self.tr("CLASSIFICATION"))
        self.classification1.lineEdit().textEdited.connect(self.updatePrintLayout)
        self.classification2.lineEdit().textEdited.connect(self.updatePrintLayout)
        self.exerciseorganisationLE.textEdited.connect(self.updatePrintLayout)
        self.coursetitleLE.textEdited.connect(self.updatePrintLayout)
        self.troopstitleLE.textEdited.connect(self.updatePrintLayout)
        self.codenameLE.textEdited.connect(self.updatePrintLayout)
        self.cartouchecircumscriptionLE.textEdited.connect(self.updatePrintLayout)
        self.supplementtitleLE.textEdited.connect(self.updatePrintLayout)
        self.scaletitleLE.textEdited.connect(self.updatePrintLayout)
        self.exercisetitleLE.textEdited.connect(self.updatePrintLayout)
        self.documenttitleLE.textEdited.connect(self.updatePrintLayout)
        self.placedateLE.textEdited.connect(self.updatePrintLayout)
        self.exerciseGroupBox.toggled.connect(self.updatePrintLayout)

        self.mapcartoucheView.setScene(self.scene)
        self.mapcartoucheView.resizeEvent = self.__resizeEvent

        exportButton = self.buttonBox.addButton(self.tr("Export"), QDialogButtonBox.ActionRole)
        importButton = self.buttonBox.addButton(self.tr("Import"), QDialogButtonBox.ActionRole)
        exportButton.clicked.connect(self.__exportCartouche)
        importButton.clicked.connect(self.__importCartouche)

        self.updateUi()

        xmlstr, ok = QgsProject.instance().readEntry("VBS-Print", "cartouche")
        if ok:
            self.__deserializeCartouche(xmlstr)

    def __resizeEvent(self, ev):
        cartouche = self.scene.itemById("mapcartouche")
        if cartouche:
            self.mapcartoucheView.fitInView(cartouche, Qt.KeepAspectRatio)

    def storeInProject(self):
        QgsProject.instance().writeEntry("VBS-Print", "cartouche", self.__serializeCartouche())

    def updateUi(self):
        self.codenameLE.setText(unicode(self.__getPrintLayoutItemText("codename")))
        self.troopstitleLE.setText(unicode(self.__getPrintLayoutItemText("troopstitle")))
        self.supplementtitleLE.setText(unicode(self.__getPrintLayoutItemText("supplementtitle")))
        self.cartouchecircumscriptionLE.setText(unicode(self.__getPrintLayoutItemText("cartouchecircumscription")))
        self.scaletitleLE.setText(unicode(self.__getPrintLayoutItemText("scaletitle")))
        self.placedateLE.setText(unicode(self.__getPrintLayoutItemText("placedate")))
        self.exerciseorganisationLE.setText(unicode(self.__getPrintLayoutItemText("exerciseorganisation")))
        self.coursetitleLE.setText(unicode(self.__getPrintLayoutItemText("coursetitle")))
        self.exercisetitleLE.setText(unicode(self.__getPrintLayoutItemText("exercisetitle")))
        self.documenttitleLE.setText(unicode(self.__getPrintLayoutItemText("documenttitle")))
        self.exercisedateLE.setDate(QDate.currentDate())

    def updatePrintLayout(self, x=None):
        self.__setPrintLayoutItemText("classification1", unicode(self.classification2.currentText()))
        self.__setPrintLayoutItemText("troopstitle", unicode(self.troopstitleLE.text()))
        self.__setPrintLayoutItemText("codename", unicode(self.codenameLE.text()))
        self.__setPrintLayoutItemText("cartouchecircumscription", unicode(self.cartouchecircumscriptionLE.text()))
        self.__setPrintLayoutItemText("supplementtitle", unicode(self.supplementtitleLE.text()))
        self.__setPrintLayoutItemText("scaletitle", unicode(self.scaletitleLE.text()))
        self.__setPrintLayoutItemText("placedate", unicode(self.placedateLE.text()))

        if self.exerciseGroupBox.isChecked():
            self.__setPrintLayoutItemText("exercisedate", unicode(self.exercisedateLE.text()))
            self.__setPrintLayoutItemText("classification2", unicode(self.classification1.currentText()))
            self.__setPrintLayoutItemText("exerciseorganisation", unicode(self.exerciseorganisationLE.text()))
            self.__setPrintLayoutItemText("coursetitle", unicode(self.coursetitleLE.text()))
            self.__setPrintLayoutItemText("exercisetitle", unicode(self.exercisetitleLE.text()))
            self.__setPrintLayoutItemText("documenttitle", unicode(self.documenttitleLE.text()))
        else:
            self.__setPrintLayoutItemText("exercisedate", "")
            self.__setPrintLayoutItemText("classification2", "")
            self.__setPrintLayoutItemText("exerciseorganisation", "")
            self.__setPrintLayoutItemText("coursetitle", "")
            self.__setPrintLayoutItemText("exercisetitle", "")
            self.__setPrintLayoutItemText("documenttitle", "")

    def __setPrintLayoutItemText(self, itemid, text):
        # redraw item everytime the text changes
        # only calling the update of QGraphicScene in the updatePrintLayout()
        # does not update the items
        item = self.scene.itemById(itemid)
        if item:
            item.setText(text)
            item.redraw()

    def __getPrintLayoutItemText(self, itemid):
        item = self.scene.itemById(itemid)
        if item:
            return item.text()
        return ""

    def __addTextElement(self, parent, element, text):
        el = parent.ownerDocument().createElement(element)
        el.appendChild(parent.ownerDocument().createTextNode(text))
        parent.appendChild(el)

    def __getElementText(self, parent, element, default=""):
        try:
            return parent.elementsByTagName(element).at(0).toElement().text()
        except Exception as e:
            return default

    def __serializeCartouche(self):
        doc = QDomDocument()
        legend = doc.createElement("Legend")
        doc.appendChild(legend)
        classification1 = self.classification1.itemData(self.classification1.findText(self.classification1.lineEdit().text())) or self.classification1.lineEdit().text() or "None"
        classification2 = self.classification2.itemData(self.classification2.findText(self.classification2.lineEdit().text())) or self.classification2.lineEdit().text() or "None"

        self.__addTextElement(legend, "ExerciseInfoVisible", ("1" if self.exerciseGroupBox.isChecked() else "0"))
        self.__addTextElement(legend, "ExerciseDate", self.exercisedateLE.date().toString("yyyy-MM-ddT00:00:00"))
        self.__addTextElement(legend, "ExerciseCommandUnit", self.exerciseorganisationLE.text())
        self.__addTextElement(legend, "ExerciseServiceContext", self.coursetitleLE.text())
        self.__addTextElement(legend, "ExerciseClassification", classification1)
        self.__addTextElement(legend, "ExerciseCodeName", self.exercisetitleLE.text())
        self.__addTextElement(legend, "ExerciseDocumentRef", self.documenttitleLE.text())
        self.__addTextElement(legend, "MissionClassification", classification2)
        self.__addTextElement(legend, "MissionUnit", self.troopstitleLE.text())
        self.__addTextElement(legend, "MissionLocation", self.placedateLE.text())
        self.__addTextElement(legend, "MissionCodeName", self.codenameLE.text())
        self.__addTextElement(legend, "PrintName", self.cartouchecircumscriptionLE.text())
        self.__addTextElement(legend, "PrintAnnexRef", self.supplementtitleLE.text())
        self.__addTextElement(legend, "PrintScaleRef", self.scaletitleLE.text())

        return doc.toString()

    def __deserializeCartouche(self, xmlstr):
        doc = QDomDocument()
        if not doc.setContent(xmlstr):
            return False

        legend = doc.documentElement()
        if legend.nodeName() != "Legend":
            return False

        try:
            self.exerciseGroupBox.setChecked(int(self.__getElementText(legend, "ExerciseInfoVisible", "0")))
        except:
            self.exerciseGroupBox.setChecked(False)

        classification1 = self.__getElementText(legend, "ExerciseClassification")
        if classification1 == "None":
            classification1 = ""
        classification2 = self.__getElementText(legend, "MissionClassification")
        if classification2 == "None":
            classification2 = ""
        classification1idx = self.classification1.findData(classification1)
        classification2idx = self.classification2.findData(classification2)

        self.exercisedateLE.setDate(QDate.fromString(self.__getElementText(legend, "ExerciseDate"), "yyyy-MM-ddT00:00:00"))
        self.exerciseorganisationLE.setText(self.__getElementText(legend, "ExerciseCommandUnit"))
        self.coursetitleLE.setText(self.__getElementText(legend, "ExerciseServiceContext"))
        if classification1idx >= 0:
            self.classification1.setCurrentIndex(classification1idx)
        else:
            self.classification1.lineEdit().setText(classification1)
        self.exercisetitleLE.setText(self.__getElementText(legend, "ExerciseCodeName"))
        self.documenttitleLE.setText(self.__getElementText(legend, "ExerciseDocumentRef"))
        if classification2idx >= 0:
            self.classification2.setCurrentIndex(classification2idx)
        else:
            self.classification2.lineEdit().setText(classification2)
        self.troopstitleLE.setText(self.__getElementText(legend, "MissionUnit"))
        self.placedateLE.setText(self.__getElementText(legend, "MissionLocation"))
        self.codenameLE.setText(self.__getElementText(legend, "MissionCodeName"))
        self.cartouchecircumscriptionLE.setText(self.__getElementText(legend, "PrintName"))
        self.supplementtitleLE.setText(self.__getElementText(legend, "PrintAnnexRef"))
        self.scaletitleLE.setText(self.__getElementText(legend, "PrintScaleRef"))
        self.updatePrintLayout()
        return True

    def __exportCartouche(self):
        lastDir = QSettings().value("/UI/lastImportExportDir", ".")
        filename = QFileDialog.getSaveFileName(self, self.tr("Export cartouche"), lastDir, self.tr("XML Files (*.xml);;"))
        if type(filename) == tuple: filename = filename[0]
        if not filename:
            return
        fileinfo = QFileInfo(filename)
        QSettings().setValue("/UI/lastImportExportDir", fileinfo.absolutePath())

        file = QFile(filename)
        if not file.open(QIODevice.WriteOnly):
            QMessageBox.critical(self, self.tr("Export failed"), self.tr("Unable to write to file."))

        file.write(self.__serializeCartouche().encode('utf-8'))

    def __importCartouche(self):
        lastDir = QSettings().value("/UI/lastImportExportDir", ".")
        filename = QFileDialog.getOpenFileName(self, self.tr("Import cartouche"), lastDir, self.tr("XML Files (*.xml);;"))
        if type(filename) == tuple: filename = filename[0]
        fileinfo = QFileInfo(filename)
        if not fileinfo.exists():
            return

        QSettings().setValue("/UI/lastImportExportDir", fileinfo.absolutePath())

        file = QFile(filename)
        if not file.open(QIODevice.ReadOnly):
            QMessageBox.critical(self, self.tr("Import failed"), self.tr("Unable to read file."))

        xmlstr = file.readAll()
        if not self.__deserializeCartouche(xmlstr):
            QMessageBox.critical(self, self.tr("Import failed"), self.tr("The file does not appear to contain valid cartouche data."))
        else:
            self.updatePrintLayout()
