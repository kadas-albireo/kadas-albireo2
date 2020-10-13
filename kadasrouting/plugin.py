# -*- coding: utf-8 -*-
import os
import logging
from functools import partial

from PyQt5.QtCore import QObject, QSettings
from PyQt5.QtWidgets import QAction

from qgis.utils import iface

from qgis.core import QgsApplication

from kadas.kadasgui import KadasPluginInterface

from kadasrouting.utilities import icon, pushWarning
from kadasrouting.core.optimalroutelayer import OptimalRouteLayerType
from kadasrouting.gui.optimalroutebottombar import OptimalRouteBottomBar
from kadasrouting.gui.reachabilitybottombar import ReachabilityBottomBar
from kadasrouting.gui.tspbottombar import TSPBottomBar
from kadasrouting.gui.navigationpanel import NavigationPanel
from kadasrouting.gui.disclaimerdialog import DisclaimerDialog
from kadasrouting.valhalla.client import ValhallaClient

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
        self.navigationPanel = None

    def initGui(self):
        # Routing menu
        self.optimalRouteAction = QAction(icon("routing.png"), self.tr("Routing"))
        self.iface.addAction(
            self.optimalRouteAction, self.iface.PLUGIN_MENU, self.iface.GPS_TAB
        )

        # Reachability menu
        self.reachabilityAction = QAction(
            icon("reachability.png"), self.tr("Reachability")
        )
        self.iface.addAction(
            self.reachabilityAction, self.iface.PLUGIN_MENU, self.iface.ANALYSIS_TAB
        )

        # TSP menu (disable TSP since it's removed from the project)
        # self.tspAction = QAction(icon("tsp.png"), self.tr("TSP"))
        # self.iface.addAction(self.tspAction, self.iface.PLUGIN_MENU, self.iface.GPS_TAB)

        # Navigation menu
        self.navigationAction = QAction(icon("navigate.png"), self.tr("Navigate"))
        self.iface.addAction(self.navigationAction, self.iface.PLUGIN_MENU, self.iface.GPS_TAB)

        self.actionsToggled = {
            self.navigationAction: self.showNavigation,
            self.reachabilityAction: self.showReachability,
            self.optimalRouteAction: self.showOptimalRoute
            # self.tspAction: self.showTSP
            }
        for action in self.actionsToggled:
            action.setCheckable(True)
            action.toggled.connect(partial(self._showPanel, action))

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
            self.navigationAction, self.iface.PLUGIN_MENU, self.iface.GPS_TAB
        )
        # self.iface.removeAction(
        #     self.tspAction, self.iface.PLUGIN_MENU, self.iface.GPS_TAB
        # )

    def _showPanel(self, action, show):
        function = self.actionsToggled[action]
        if show:
            self._hidePanels(action)
        function(show)

    def _hidePanels(self, keep=None):
        for action in self.actionsToggled:
            if action != keep:
                action.setChecked(False)

    def showOptimalRoute(self, show=True):
        if show:
            if self.optimalRouteBar is None:
                self.optimalRouteBar = OptimalRouteBottomBar(
                    self.iface.mapCanvas(),
                    self.optimalRouteAction,
                    self
                )
            self.showDisclaimer()
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
            self.showDisclaimer()
            self.reachabilityBar.show()
        else:
            if self.reachabilityBar is not None:
                self.reachabilityBar.hide()

    def showTSP(self, show=True):
        if show:
            if self.tspBar is None:
                self.tspBar = TSPBottomBar(self.iface.mapCanvas(), self.tspAction)
            self.showDisclaimer()
            self.tspBar.show()
        else:
            if self.tspBar is not None:
                self.tspBar.hide()

    def showNavigation(self, show=True):
        if show:
            if self.navigationPanel is None:
                self.navigationPanel = NavigationPanel()

                def _resize():
                    x = self.iface.mapCanvas().width() - self.navigationPanel.FIXED_WIDTH
                    y = self.iface.mapCanvas().height() / 3
                    height = 2 * self.iface.mapCanvas().height() / 3
                    self.navigationPanel.setGeometry(x, y, self.navigationPanel.FIXED_WIDTH, height)
                self.iface.mapCanvas().extentsChanged.connect(_resize)
                self.navigationPanel.setParent(self.iface.mapCanvas())
                _resize()
            self.showDisclaimer()
            self.navigationPanel.show()
        else:
            if self.navigationPanel is not None:
                self.navigationPanel.hide()

    def showDisclaimer(self):
        show = QSettings().value("kadasrouting/showDisclaimer", True, type=bool)
        if show:
            dialog = DisclaimerDialog(iface.mainWindow())
            dialog.exec_()

def testclientavailability(method):
    def func(*args, **kw):
        if ValhallaClient.getInstance().isAvailable():            
            method(*args, **kw)
        else:
            pushWarning(self.tr("Valhalla is not installed or it cannot be found"))
    return func