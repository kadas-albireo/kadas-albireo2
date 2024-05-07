# -*- coding: utf-8 -*-
"""
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
"""

import os

from qgis.core import *
from qgis.PyQt.QtCore import *
from qgis.PyQt.QtGui import *
from qgis.PyQt.QtWidgets import *
try:
    from PyQt5.QtWebKitWidgets import QWebView, QWebPage
    HAVE_WEBKIT = True
except Exception as e:
    HAVE_WEBKIT = False
from kadas.kadascore import *
from kadas.kadasgui import *


class SearchField(QLineEdit):
    escapePressed = pyqtSignal()

    def __init__(self):
        QLineEdit.__init__(self)

    def keyPressEvent(self, event):
        if event.key() == Qt.Key_Escape:
            self.escapePressed.emit()
            event.accept()
        else:
            QLineEdit.keyPressEvent(self, event)


class HelpPlugin:

    def __init__(self, iface):
        self.iface = KadasPluginInterface.cast(iface)
        self.plugin_dir = os.path.dirname(__file__)

        # initialize locale
        self.locale = QSettings().value('locale/userLocale', 'en')[0:2]
        locale_path = os.path.join(
            self.plugin_dir,
            'i18n',
            'KadasHelp_{}.qm'.format(self.locale))

        if os.path.exists(locale_path):
            self.translator = QTranslator()
            self.translator.load(locale_path)

            if qVersion() > '4.3.3':
                QCoreApplication.installTranslator(self.translator)

        self.helpWindow = None
        self.helpWidget = None
        self.searchWidget = None
        self.searchField = None
        self.searchShortcut = None
        self.helpAction = None
        self.timer = None

    def tr(self, message):
        return QCoreApplication.translate('UserManual', message)

    def initGui(self):
        docdir = os.path.join(self.plugin_dir, "html")
        self.iface.mainWindowClosed.connect(self.closeHelpWindow)

        self.server = KadasFileServer(docdir, "127.0.0.1")
        QgsLogger.debug("Help server running on {host}:{port}".format(host=self.server.getHost(), port=self.server.getPort()))
        self.helpAction = self.iface.findAction("mActionHelp")
        self.helpAction.triggered.connect(self.showHelp)

        if os.path.isdir(os.path.join(docdir, self.locale)):
            self.lang = self.locale
        else:
            self.lang = 'en'

    def unload(self):
        self.helpWindow = None
        self.helpWidget = None
        self.searchWidget = None
        self.searchField = None
        self.searchShortcut = None
        self.helpAction = None
        self.timer = None

    def showHelp(self):
        docdir = os.path.join(self.plugin_dir, "html")


        url = QUrl("http:///{host}:{port}/{lang}/".format(
            host=self.server.getHost(), port=self.server.getPort(), lang=self.lang))

        if not HAVE_WEBKIT:
            QDesktopServices.openUrl(url)
        else:
            if not self.helpWindow:
                self.helpWindow = QWidget()
                self.helpWindow.setWindowTitle(self.tr('KADAS User Manual'))
                self.helpWindow.resize(1024, 768)
                self.helpWindow.setLayout(QVBoxLayout())
                self.helpWindow.layout().setContentsMargins(0, 0, 0, 0)

                self.helpWidget = QWebView()
                self.helpWidget.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
                self.helpWindow.layout().addWidget(self.helpWidget)

                self.searchWidget = QWidget()
                self.searchWidget.setLayout(QHBoxLayout())
                self.searchWidget.layout().setContentsMargins(2, 0, 2, 2)
                self.searchWidget.setVisible(False)

                self.searchField = SearchField()
                self.searchField.setPlaceholderText(self.tr("Search..."))
                self.searchField.textChanged.connect(self.search)
                self.searchField.returnPressed.connect(self.search)
                self.searchField.escapePressed.connect(lambda: self.searchWidget.setVisible(False))
                self.searchWidget.layout().addWidget(self.searchField)

                searchNextButton = QPushButton(self.tr("Next"))
                searchNextButton.setIcon(QgsApplication.getThemeIcon("/mActionArrowDown.svg"))
                searchNextButton.clicked.connect(lambda: self.search(True))
                self.searchWidget.layout().addWidget(searchNextButton)

                searchPrevButton = QPushButton(self.tr("Previous"))
                searchPrevButton.setIcon(QgsApplication.getThemeIcon("/mActionArrowUp.svg"))
                searchPrevButton.clicked.connect(lambda: self.search(False))
                self.searchWidget.layout().addWidget(searchPrevButton)

                closeButton = QPushButton()
                closeButton.setIcon(QIcon(":/kadas/icons/close"))
                closeButton.clicked.connect(lambda: self.searchWidget.setVisible(False))
                self.searchWidget.layout().addWidget(closeButton)

                self.helpWindow.layout().addWidget(self.searchWidget)

                self.searchShortcut = QShortcut( QKeySequence( Qt.CTRL + Qt.Key_F ), self.helpWindow);
                self.searchShortcut.activated.connect(self.showSearch)

                self.timer = QTimer()
                self.timer.setInterval(250)
                self.timer.setSingleShot(True)
                self.timer.timeout.connect(self.raiseHelpWindow)
            self.helpWidget.load(url)
            self.timer.start()

    def showSearch(self):
        self.searchField.setText("")
        self.searchWidget.setVisible(True)
        self.searchField.setFocus()

    def search(self, searchNext=True):
        flags = QWebPage.FindWrapsAroundDocument
        if searchNext == False:
            flags |= QWebPage.FindBackward
        self.helpWidget.page().findText(self.searchField.text(), flags)

    def raiseHelpWindow(self):
            self.helpWindow.show()
            self.helpWindow.raise_()

    def closeHelpWindow(self):
        if self.helpWindow:
            self.helpWindow.close()
