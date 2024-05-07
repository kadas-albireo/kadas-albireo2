# -*- coding: utf-8 -*-
#
#    This program is free software; you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation; either version 2 of the License, or
#    (at your option) any later version.
#
#    copyright            : (C) 2014-2015 by Sandro Mani / Sourcepole AG
#    email                : smani@sourcepole.ch

from qgis.PyQt.QtCore import *
from qgis.PyQt.QtGui import *
from qgis.PyQt.QtWidgets import *
from qgis.core import *
from qgis.gui import *
from kadas.kadasgui import *
import os

from .PrintTool import PrintTool
from . import resources_rc


class PrintPlugin(QObject):
    def __init__(self, iface):
        QObject.__init__(self)
        # Save reference to the QGIS interface and Kadas interface
        self.iface = KadasPluginInterface.cast(iface)

        self.pluginDir = os.path.dirname(__file__)

        # Localize
        if QSettings().value("locale/userLocale"):
            locale = QSettings().value("locale/userLocale")[0:2]
            localePath = os.path.join(
                self.pluginDir, 'i18n', 'print_{}.qm'.format(locale))

            if os.path.exists(localePath):
                self.translator = QTranslator()
                self.translator.load(localePath)
                QCoreApplication.installTranslator(self.translator)

        self.toolAction = None

    def initGui(self):
        try:
            self.printAction = self.iface.findAction("mActionPrint")
        except:
            self.printAction = None
        if self.printAction:
            self.printAction.setCheckable(True)
            self.printAction.triggered.connect(self.__toggleTool)

    def unload(self):
        pass

    def __toggleTool(self, active):
        if active:
            self.tool = PrintTool(self.iface)
            self.tool.setAction(self.printAction)
            self.iface.mapCanvas().setMapTool(self.tool)
        elif self.tool:
            self.iface.mapCanvas().unsetMapTool(self.tool)
            self.tool = None
