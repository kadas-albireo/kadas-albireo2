# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'printlayoutmanager.ui'
#
# Created by: PyQt5 UI code generator 5.12.3
#
# WARNING! All changes made in this file will be lost!


from PyQt5 import QtCore, QtGui, QtWidgets


class Ui_PrintLayoutManager(object):
    def setupUi(self, PrintLayoutManager):
        PrintLayoutManager.setObjectName("PrintLayoutManager")
        PrintLayoutManager.resize(400, 300)
        self.gridLayout = QtWidgets.QGridLayout(PrintLayoutManager)
        self.gridLayout.setObjectName("gridLayout")
        self.pushButtonImport = QtWidgets.QPushButton(PrintLayoutManager)
        self.pushButtonImport.setObjectName("pushButtonImport")
        self.gridLayout.addWidget(self.pushButtonImport, 1, 0, 1, 1)
        self.pushButtonExport = QtWidgets.QPushButton(PrintLayoutManager)
        self.pushButtonExport.setEnabled(False)
        self.pushButtonExport.setObjectName("pushButtonExport")
        self.gridLayout.addWidget(self.pushButtonExport, 1, 1, 1, 1)
        self.pushButtonRemove = QtWidgets.QPushButton(PrintLayoutManager)
        self.pushButtonRemove.setEnabled(False)
        self.pushButtonRemove.setObjectName("pushButtonRemove")
        self.gridLayout.addWidget(self.pushButtonRemove, 1, 2, 1, 1)
        self.listWidgetLayouts = QtWidgets.QListWidget(PrintLayoutManager)
        self.listWidgetLayouts.setObjectName("listWidgetLayouts")
        self.gridLayout.addWidget(self.listWidgetLayouts, 0, 0, 1, 3)
        self.buttonBox = QtWidgets.QDialogButtonBox(PrintLayoutManager)
        self.buttonBox.setOrientation(QtCore.Qt.Horizontal)
        self.buttonBox.setStandardButtons(QtWidgets.QDialogButtonBox.Close)
        self.buttonBox.setObjectName("buttonBox")
        self.gridLayout.addWidget(self.buttonBox, 2, 0, 1, 3)

        self.retranslateUi(PrintLayoutManager)
        self.buttonBox.accepted.connect(PrintLayoutManager.accept)
        self.buttonBox.rejected.connect(PrintLayoutManager.reject)
        QtCore.QMetaObject.connectSlotsByName(PrintLayoutManager)

    def retranslateUi(self, PrintLayoutManager):
        _translate = QtCore.QCoreApplication.translate
        PrintLayoutManager.setWindowTitle(_translate("PrintLayoutManager", "Print Layout Manager"))
        self.pushButtonImport.setText(_translate("PrintLayoutManager", "Import"))
        self.pushButtonExport.setText(_translate("PrintLayoutManager", "Export"))
        self.pushButtonRemove.setText(_translate("PrintLayoutManager", "Remove"))
