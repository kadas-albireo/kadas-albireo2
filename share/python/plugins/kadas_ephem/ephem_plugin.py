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
from qgis.PyQt.QtCore import QSettings, QTranslator, QCoreApplication
from qgis.PyQt.QtGui import QIcon
from qgis.PyQt.QtWidgets import QAction
from kadas.kadasgui import KadasPluginInterface
from .ephem_tool import EphemTool


class EphemPlugin:

    def __init__(self, iface):
        self.iface = KadasPluginInterface.cast(iface)
        self.plugin_dir = os.path.dirname(__file__)
        self.ephem_tool = None

        # initialize locale
        self.locale = QSettings().value('locale/userLocale', 'en')[0:2]
        locale_path = os.path.join(
            self.plugin_dir,
            'i18n',
            'kadas_ephem_{}.qm'.format(self.locale))

        if os.path.exists(locale_path):
            self.translator = QTranslator()
            self.translator.load(locale_path)
            QCoreApplication.installTranslator(self.translator)

    def tr(self, message):
        return QCoreApplication.translate('Ephem', message)

    def initGui(self):
        icon = QIcon(os.path.join(os.path.dirname(__file__), 'icons/icon.png'))
        self.action = QAction(icon, self.tr(u'Ephemeris'),
                              self.iface.mainWindow())
        self.action.setCheckable(True)
        self.action.toggled.connect(self.toolToggled)
        self.iface.addAction(self.action, self.iface.PLUGIN_MENU,
                             self.iface.ANALYSIS_TAB)

        self.iface.addActionMapCanvasRightClick(self.action)

    def unload(self):
        self.iface.removeAction(self.action, self.iface.PLUGIN_MENU,
                                self.iface.ANALYSIS_TAB)

        self.iface.removeActionMapCanvasRightClick(self.action)

    def toolToggled(self, active):
        if active:
            self.ephem_tool = EphemTool(self.iface)
            self.ephem_tool.setAction(self.action)
            self.iface.mapCanvas().setMapTool(self.ephem_tool)
        elif self.iface.mapCanvas().mapTool() and self.iface.mapCanvas().mapTool().action() == self.action:
            self.iface.mapCanvas().unsetMapTool(self.iface.mapCanvas().mapTool())
            self.ephem_tool = None
