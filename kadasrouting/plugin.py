# -*- coding: utf-8 -*-
import os
import logging

from PyQt5.QtCore import QObject
from PyQt5.QtGui import QIcon
from PyQt5.QtWidgets import QAction

from qgis.utils import iface

from kadas.kadasgui import KadasPluginInterface

from kadasrouting.utilities import icon
from kadasrouting.gui.shortestpathbottombar import ShortestPathBottomBar

logfile = os.path.join(os.path.expanduser("~"), ".kadas", "kadas-routing.log")
try:
    os.mkdir(os.path.dirname(logfile))
except FileExistsError:
    pass
logging.basicConfig(filename=logfile,level=logging.DEBUG)


class RoutingPlugin(QObject):

    def __init__(self, iface):
        QObject.__init__(self)
        
        self.iface = KadasPluginInterface.cast(iface)
        self.shortestPathBar = None

    def initGui(self):
        # Routing menu
        self.shortestAction = QAction(icon("routing.png"), self.tr("Routing"))
        self.shortestAction.setCheckable(True)
        self.shortestAction.toggled.connect(self.showShortest)
        self.iface.addAction(self.shortestAction, self.iface.PLUGIN_MENU, self.iface.GPS_TAB)

    def unload(self):
        self.iface.removeAction( self.shortestAction, self.iface.PLUGIN_MENU, self.iface.GPS_TAB)

    def showShortest(self, show=True):
        if show:
            if self.shortestPathBar is None:
                self.shortestPathBar = ShortestPathBottomBar(self.iface.mapCanvas(), self.shortestAction)
            self.shortestPathBar.show()
        else:
            if self.shortestPathBar is not None:
                self.shortestPathBar.hide()



