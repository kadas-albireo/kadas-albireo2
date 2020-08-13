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
from kadasrouting.gui.optimalroutebottombar import OptimalRouteBottomBar
from kadasrouting.gui.reachabilitybottombar import ReachabilityBottomBar
from kadasrouting.gui.tspbottombar import TSPBottomBar
from kadasrouting.gui.navigationbottombar import NavigationBottomBar

logfile = os.path.join(os.path.expanduser("~"), ".kadas", "kadas-routing.log")
try:
    os.mkdir(os.path.dirname(logfile))
except FileExistsError:
    pass
logging.basicConfig(filename=logfile, level=logging.DEBUG)


class RoutingPlugin(QObject):
    def __init__(self, iface):
        QObject.__init__(self)

        self.iface = KadasPluginInterface.cast(iface)
        self.optimalRouteBar = None
        self.reachabilityBar = None
        self.tspBar = None
        self.navigationBar = None

    def initGui(self):
        # Routing menu
        self.optimalRouteAction = QAction(icon("routing.png"), self.tr("Routing"))
        self.optimalRouteAction.setCheckable(True)
        self.optimalRouteAction.toggled.connect(self.showOptimalRoute)
        self.iface.addAction(
            self.optimalRouteAction, self.iface.PLUGIN_MENU, self.iface.GPS_TAB
        )

        # Reachability menu
        self.reachabilityAction = QAction(
            icon("reachability.png"), self.tr("Reachability")
        )
        self.reachabilityAction.setCheckable(True)
        self.reachabilityAction.toggled.connect(self.showReachability)
        self.iface.addAction(
            self.reachabilityAction, self.iface.PLUGIN_MENU, self.iface.ANALYSIS_TAB
        )

        # TSP menu
        self.tspAction = QAction(icon("tsp.png"), self.tr("TSP"))
        self.tspAction.setCheckable(True)
        self.tspAction.toggled.connect(self.showTSP)
        self.iface.addAction(self.tspAction, self.iface.PLUGIN_MENU, self.iface.GPS_TAB)

        # Navigation menu
        self.navigationAction = QAction(icon("navigate.png"), self.tr("Navigate"))
        self.navigationAction.setCheckable(True)
        self.navigationAction.toggled.connect(self.showNavigation)
        self.iface.addAction(self.navigationAction, self.iface.PLUGIN_MENU, self.iface.GPS_TAB)

        reg = QgsApplication.pluginLayerRegistry()
        reg.addPluginLayerType(OptimalRouteLayerType())

    def unload(self):
        self.iface.removeAction(
            self.optimalRouteAction, self.iface.PLUGIN_MENU, self.iface.GPS_TAB
        )
        self.iface.removeAction(
            self.reachabilityAction, self.iface.PLUGIN_MENU, self.iface.ANALYSIS_TAB
        )
        self.iface.removeAction(
            self.tspAction, self.iface.PLUGIN_MENU, self.iface.GPS_TAB
        )

    def showOptimalRoute(self, show=True):
        if show:
            if self.optimalRouteBar is None:
                self.optimalRouteBar = OptimalRouteBottomBar(
                    self.iface.mapCanvas(),
                    self.optimalRouteAction,
                    self
                )
            self.optimalRouteBar.show()
        else:
            if self.optimalRouteBar is not None:
                self.optimalRouteBar.hide()

    def showReachability(self, show=True):
        if show:
            if self.reachabilityBar is None:
                self.reachabilityBar = ReachabilityBottomBar(
                    self.iface.mapCanvas(), self.reachabilityAction
                )
            self.reachabilityBar.show()
        else:
            if self.reachabilityBar is not None:
                self.reachabilityBar.hide()

    def showTSP(self, show=True):
        if show:
            if self.tspBar is None:
                self.tspBar = TSPBottomBar(self.iface.mapCanvas(), self.tspAction)
            self.tspBar.show()
        else:
            if self.tspBar is not None:
                self.tspBar.hide()

    def showNavigation(self, show=True):
        if show:
            if self.navigationBar is None:
                self.navigationBar = NavigationBottomBar(
                    self.iface.mapCanvas(), self.navigationAction)
            self.navigationBar.show()
        else:
            if self.navigationBar is not None:
                self.navigationBar.hide()