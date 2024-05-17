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
from qgis.PyQt.QtCore import *
from qgis.PyQt.QtGui import *
from qgis.PyQt.QtWidgets import *
from kadas.kadasgui import *
from . import resources
from .about_dialog import AboutDialog


class AboutPlugin:

    def __init__(self, iface):
        self.iface = KadasPluginInterface.cast(iface)
        self.plugin_dir = os.path.dirname(__file__)

        # initialize locale
        self.locale = QSettings().value('locale/userLocale', 'en')[0:2]
        locale_path = os.path.join(
            self.plugin_dir,
            'i18n',
            'KadasAbout_{}.qm'.format(self.locale))

        if os.path.exists(locale_path):
            self.translator = QTranslator()
            self.translator.load(locale_path)
            QCoreApplication.installTranslator(self.translator)

        self.aboutWidget = None
        self.aboutAction = None

    def initGui(self):
        self.aboutAction = self.iface.findAction("mActionAbout")
        self.aboutAction.triggered.connect(self.showAbout)

    def unload(self):
        self.aboutWidget = None
        self.aboutAction = None

    def showAbout(self):
        locale = self.locale
        if not locale in ['en', 'de', 'it', 'fr']:
            locale = 'en'
        if self.aboutWidget is None:
            self.aboutWidget = AboutDialog(locale, self.iface.mainWindow())
        self.aboutWidget.show()
        self.aboutWidget.raise_()
