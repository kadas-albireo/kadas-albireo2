# -*- coding: utf-8 -*-

from qgis.PyQt.QtCore import *
from qgis.PyQt.QtGui import *
from qgis.PyQt.QtWidgets import *
import os
import sys

from . import resources
from .kadas_gpkg_export import KadasGpkgExport
from .kadas_gpkg_import import KadasGpkgImport
from qgis.gui import *
from kadas.kadasgui import *


class KadasGpkgDropHandler(QgsCustomDropHandler):
    def __init__(self, iface):
        QgsCustomDropHandler.__init__(self)
        self.iface = iface

    def canHandleMimeData(self, mimedata):
        for url in mimedata.urls():
            if url.toLocalFile().lower().endswith(".gpkg"):
                return True
        return False

    def handleMimeDataV2( self, mimedata):
        if len(mimedata.urls()) > 0:
            path = mimedata.urls()[0].toLocalFile()
            if path.lower().endswith(".gpkg"):
                KadasGpkgImport(self.iface).run(path)
                return True
        return False


class KadasGpkg(QObject):

    def __init__(self, iface):
        QObject.__init__(self)

        self.iface = KadasPluginInterface.cast(iface)
        self.dropHandler = KadasGpkgDropHandler(self.iface)

        # initialize locale
        if QSettings().value('locale/userLocale'):
            self.locale = QSettings().value('locale/userLocale')[0:2]
            locale_path = os.path.join(
                os.path.dirname(__file__),
                'i18n',
                'kadas_gpkg_{}.qm'.format(self.locale))

            if os.path.exists(locale_path):
                self.translator = QTranslator()
                self.translator.load(locale_path)
                QCoreApplication.installTranslator(self.translator)

        self.kadasGpkgExport = KadasGpkgExport(self.iface)

    def initGui(self):

        self.menu = QMenu()

        self.exportShortcut = QShortcut(QKeySequence(Qt.CTRL + Qt.Key_E, Qt.CTRL + Qt.Key_G), self.iface.mainWindow())
        self.exportShortcut.activated.connect(self.__exportGpkg)
        self.exportAction = QAction(self.tr("GPKG Export"))
        self.exportAction.triggered.connect(self.__exportGpkg)
        self.menu.addAction(self.exportAction)

        self.importShortcut = QShortcut(QKeySequence(Qt.CTRL + Qt.Key_I, Qt.CTRL + Qt.Key_G), self.iface.mainWindow())
        self.importShortcut.activated.connect(self.__importGpkg)
        self.importAction = QAction(self.tr("GPKG Import"))
        self.importAction.triggered.connect(self.__importGpkg)
        self.menu.addAction(self.importAction)

        self.iface.addActionMenu(self.tr("GPKG"),
                                 QIcon(":/plugins/KADASGpkg/icons/gpkg.png"),
                                 self.menu,
                                 self.iface.PLUGIN_MENU,
                                 self.iface.MAPS_TAB)
        self.iface.registerCustomDropHandler(self.dropHandler)

    def unload(self):
        self.iface.removeActionMenu(self.menu, self.iface.PLUGIN_MENU, self.iface.MAPS_TAB)
        self.iface.unregisterCustomDropHandler(self.dropHandler)

    def __importGpkg(self):
        KadasGpkgImport(self.iface).run()

    def __exportGpkg(self):
        self.kadasGpkgExport.run()
