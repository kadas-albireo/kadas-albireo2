# -*- coding: utf-8 -*-
import os
import logging
from functools import partial

from PyQt5.QtCore import QObject, QSettings
from PyQt5.QtWidgets import QAction

from qgis.utils import iface

from qgis.core import QgsApplication

from kadas.kadasgui import KadasPluginInterface

from kadas_routing.utilities import icon, pushWarning, tr
from kadas_routing.core.optimalroutelayer import OptimalRouteLayerType
from kadas_routing.gui.optimalroutebottombar import OptimalRouteBottomBar
from kadas_routing.gui.cpbottombar import CPBottomBar
from kadas_routing.gui.reachabilitybottombar import ReachabilityBottomBar
from kadas_routing.gui.datacataloguebottombar import DataCatalogueBottomBar
from kadas_routing.gui.navigationpanel import NavigationPanel
from kadas_routing.gui.disclaimerdialog import DisclaimerDialog
from kadas_routing.valhalla.client import ValhallaClient

from kadas_routing.core.memorylayersaver import MemoryLayerSaver

from kadas_routing.core.datacatalogueclient import DataCatalogueClient


logfile = os.path.join(os.path.expanduser("~"), ".kadas", "kadas-routing.log")
try:
    os.mkdir(os.path.dirname(logfile))
except FileExistsError:
    pass
logging.basicConfig(filename=logfile, level=logging.DEBUG)

LOG = logging.getLogger(__name__)


def testclientavailability(method):
    def func(*args, **kw):
        if ValhallaClient.getInstance().isAvailable():
            method(*args, **kw)
        else:
            pushWarning(tr("Valhalla is not installed or it cannot be found"))

    return func


class RoutingPlugin(QObject):
    def __init__(self, iface):
        QObject.__init__(self)

        self.iface = KadasPluginInterface.cast(iface)
        self.optimalRouteBar = None
        self.cpBar = None
        self.reachabilityBar = None
        self.dataCatalogueBar = None
        self.navigationPanel = None

        # auto saver for memory layers
        self._saver = MemoryLayerSaver(iface)

    def initGui(self):
        # Routing menu
        self.optimalRouteAction = QAction(icon("routing.png"), self.tr("Routing"))
        self.iface.addAction(
            self.optimalRouteAction, self.iface.PLUGIN_MENU, self.iface.GPS_TAB
        )

        # Chinese Postaman menu
        self.cpAction = QAction(icon("chinesepostman.png"), self.tr("Patrol") + "\n(Beta)")
        self.iface.addAction(self.cpAction, self.iface.PLUGIN_MENU, self.iface.GPS_TAB)

        # Reachability menu
        self.reachabilityAction = QAction(
            icon("reachability.png"), self.tr("Reachability")
        )
        self.iface.addAction(
            self.reachabilityAction, self.iface.PLUGIN_MENU, self.iface.ANALYSIS_TAB
        )

        # Navigation menu
        self.navigationAction = QAction(icon("navigate.png"), self.tr("Navigate"))
        
        self.iface.addAction(
            self.navigationAction, self.iface.PLUGIN_MENU, self.iface.GPS_TAB
        )

        # Day and Night
        self.dayNightAction = QAction(icon("day-and-night.png"), self.tr("Day / Night"))
        # Removed until we have one
        # self.iface.addAction(
        #     self.dayNightAction, self.iface.PLUGIN_MENU, self.iface.GPS_TAB
        # )

        self.dataCatalogueAction = QAction(
            icon("data-catalogue.png"), self.tr("Routing Data")
        )
        self.iface.addAction(
            self.dataCatalogueAction, self.iface.PLUGIN_MENU, self.iface.GPS_TAB
        )

        self.actionsToggled = {
            self.navigationAction: self.showNavigation,
            self.reachabilityAction: self.showReachability,
            self.optimalRouteAction: self.showOptimalRoute,
            self.cpAction: self.showCP,
            self.dataCatalogueAction: self.showDataCatalogue,
        }

        # Handling unique active action
        for action in self.actionsToggled:
            action.setCheckable(True)
            action.toggled.connect(partial(self._showPanel, action))

        # Day Night action is independent
        self.dayNightAction.setCheckable(True)
        self.dayNightAction.toggled.connect(self.toggleDayNight)

        reg = QgsApplication.pluginLayerRegistry()
        reg.addPluginLayerType(OptimalRouteLayerType())

        # auto saver for memory layers
        self._saver.attachToProject()

        self.iface.getRibbonWidget().currentChanged.connect(self._hidePanels)

        self.dataCatalogueBar = DataCatalogueBottomBar(
                    self.iface.mapCanvas(), self.dataCatalogueAction
                )
        
        self.dataCatalogueBar.dataCatalogueClient.data_changed.connect(self.catalogDataChanged)
        
        self.catalogDataChanged()
        
        self.dataCatalogueBar.hide()
    
    def catalogDataChanged(self):
        if self.dataCatalogueBar.radioButtonGroup.checkedButton() == None:
            #pushWarning("checkedButton: None")
            self.enableRoutingMenus(False)
        else:
            btn = self.dataCatalogueBar.radioButtonGroup.button(self.dataCatalogueBar.radioButtonGroup.checkedId())

            if not btn.isChecked():
                #pushWarning("Button not enabled")
                self.enableRoutingMenus(False)
            else:
                #pushWarning("Button enabled")
                self.enableRoutingMenus(True)
    
    def enableRoutingMenus(self, val):
        self.optimalRouteAction.setEnabled(val)
        self.cpAction.setEnabled(val)
        self.reachabilityAction.setEnabled(val)
        self.navigationAction.setEnabled(val)
    
    def unload(self):
        self.iface.removeAction(
            self.optimalRouteAction, self.iface.PLUGIN_MENU, self.iface.GPS_TAB
        )
        self.iface.removeAction(
            self.cpAction, self.iface.PLUGIN_MENU, self.iface.GPS_TAB
        )
        self.iface.removeAction(
            self.reachabilityAction, self.iface.PLUGIN_MENU, self.iface.ANALYSIS_TAB
        )
        self.iface.removeAction(
            self.navigationAction, self.iface.PLUGIN_MENU, self.iface.GPS_TAB
        )
        self.iface.removeAction(
            self.dataCatalogueAction, self.iface.PLUGIN_MENU, self.iface.GPS_TAB
        )
        self.iface.removeAction(
            self.dayNightAction, self.iface.PLUGIN_MENU, self.iface.GPS_TAB
        )
        self._saver.detachFromProject()

    def _showPanel(self, action, show):
        function = self.actionsToggled[action]
        if show:
            self._hidePanels(action)
        function(show)

    def _hidePanels(self, keep=None):
        for action in self.actionsToggled:
            if action != keep:
                action.setChecked(False)
    
    @testclientavailability
    def showOptimalRoute(self, show=True):
        if show:
            if self.optimalRouteBar is None:
                self.optimalRouteBar = OptimalRouteBottomBar(
                    self.iface.mapCanvas(), self.optimalRouteAction, self
                )
            self.showDisclaimer()
            self.optimalRouteBar.show()
        else:
            if self.optimalRouteBar is not None:
                self.optimalRouteBar.hide()
                
        if self.optimalRouteBar is not None:
            self.optimalRouteBar.resetCombo(show)

    @testclientavailability
    def showCP(self, show=True):
        if show:
            if self.cpBar is None:
                self.cpBar = CPBottomBar(self.iface.mapCanvas(), self.cpAction, self)
            self.showDisclaimer()
            self.cpBar.show()
        else:
            if self.cpBar is not None:
                self.cpBar.hide()
        
        if self.cpBar is not None:        
            self.cpBar.resetCombo(show)
                
    @testclientavailability
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

    @testclientavailability
    def showNavigation(self, show=True):
        if show:
            if self.navigationPanel is None:
                self.navigationPanel = NavigationPanel()

                def _resize():
                    x = int(
                        self.iface.mapCanvas().width()
                        - self.navigationPanel.FIXED_WIDTH
                    )
                    y = int(self.iface.mapCanvas().height() / 3)
                    height = int(2 * self.iface.mapCanvas().height() / 3)
                    self.navigationPanel.setGeometry(
                        x, y, int(self.navigationPanel.FIXED_WIDTH), height
                    )

                self.iface.mapCanvas().extentsChanged.connect(_resize)
                self.navigationPanel.setParent(self.iface.mapCanvas())
                _resize()
            self.showDisclaimer()
            self.navigationPanel.show()
        else:
            if self.navigationPanel is not None:
                self.navigationPanel.hide()

    def showDataCatalogue(self, show=True):
        if show:
            if self.dataCatalogueBar is None:
                self.dataCatalogueBar = DataCatalogueBottomBar(
                    self.iface.mapCanvas(), self.dataCatalogueAction
                )
            self.showDisclaimer()
            self.dataCatalogueBar.show()
        else:
            if self.dataCatalogueBar is not None:
                self.dataCatalogueBar.hide()

    def showDisclaimer(self):
        show = QSettings().value("kadasrouting/showDisclaimer", True, type=bool)
        if show:
            dialog = DisclaimerDialog(iface.mainWindow())
            dialog.exec_()

    def toggleDayNight(self, day=True):
        if day:
            pushWarning("Show day map")
        else:
            pushWarning("show night map")
