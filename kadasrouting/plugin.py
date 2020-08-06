# -*- coding: utf-8 -*-
import os
import logging

from PyQt5.QtCore import QObject
from PyQt5.QtGui import QIcon
from PyQt5.QtWidgets import QAction

from qgis.utils import iface

from qgis.core import QgsPluginLayerRegistry, QgsApplication

from kadas.kadasgui import KadasPluginInterface

from kadasrouting.utilities import icon, pushWarning
from kadasrouting.core.optimalroutelayer import OptimalRouteLayerType
from kadasrouting.core.isochroneslayer import IsochronesLayerType
from kadasrouting.gui.optimalroutebottombar import OptimalRouteBottomBar
from kadasrouting.gui.reachibilitybottombar import ReachibilityBottomBar
from kadasrouting.gui.tspbottombar import TSPBottomBar

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
        self.optimalRouteBar = None
        self.reachibilityBar = None
        self.tspBar = None

    def initGui(self):
        # Routing menu
        self.optimalRouteAction = QAction(icon("routing.png"), self.tr("Routing"))
        self.optimalRouteAction.setCheckable(True)
        self.optimalRouteAction.toggled.connect(self.showOptimalRoute)
        self.iface.addAction(self.optimalRouteAction, self.iface.PLUGIN_MENU, self.iface.GPS_TAB)

        # Reachibility menu
        self.reachabilityAction = QAction(icon("reachibility.png"), self.tr("Reachability"))
        self.reachabilityAction.setCheckable(True)
        self.reachabilityAction.toggled.connect(self.showReachibility)
        self.iface.addAction(self.reachabilityAction, self.iface.PLUGIN_MENU, self.iface.ANALYSIS_TAB)

        # TSP menu
        self.tspAction = QAction(icon("tsp.png"), self.tr("TSP"))
        self.tspAction.setCheckable(True)
        self.tspAction.toggled.connect(self.showTSP)
        self.iface.addAction(self.tspAction, self.iface.PLUGIN_MENU, self.iface.GPS_TAB)

        reg = QgsApplication.pluginLayerRegistry()
        reg.addPluginLayerType(OptimalRouteLayerType())
        reg.addPluginLayerType(IsochronesLayerType())

    def unload(self):
        self.iface.removeAction(self.optimalRouteAction, self.iface.PLUGIN_MENU, self.iface.GPS_TAB)
        self.iface.removeAction(self.reachabilityAction, self.iface.PLUGIN_MENU, self.iface.ANALYSIS_TAB)
        self.iface.removeAction(self.tspAction, self.iface.PLUGIN_MENU, self.iface.GPS_TAB)

    def showOptimalRoute(self, show=True):
        if show:
            if self.optimalRouteBar is None:
                self.optimalRouteBar = OptimalRouteBottomBar(self.iface.mapCanvas(), self.optimalRouteAction)
            self.optimalRouteBar.show()
        else:
            if self.optimalRouteBar is not None:
                self.optimalRouteBar.hide()

    def showReachibility(self, show=True):
        if show:
            if self.reachibilityBar is None:
                self.reachibilityBar = ReachibilityBottomBar(self.iface.mapCanvas(), self.reachabilityAction)
            self.reachibilityBar.show()
        else:
            if self.reachibilityBar is not None:
                self.reachibilityBar.hide()


    def showTSP(self, show=True):
        if show:
            if self.tspBar is None:
                self.tspBar = TSPBottomBar(self.iface.mapCanvas(), self.tspAction)
            self.tspBar.show()
        else:
            if self.tspBar is not None:
                self.tspBar.hide()
