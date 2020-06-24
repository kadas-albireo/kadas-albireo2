import os
from PyQt5 import uic
from PyQt5.QtGui import QIcon
from kadas.kadasgui import (
    KadasBottomBar, 
    KadasSearchBox, 
    KadasCoordinateSearchProvider, 
    KadasLocationSearchProvider, 
    KadasLocalDataSearchProvider,
    KadasPinSearchProvider,
    KadasRemoteDataSearchProvider,
    KadasWorldLocationSearchProvider)

from kadasrouting.utilities import icon

from kadasrouting.gui.routingsearchbox import addSearchBox

WIDGET, BASE = uic.loadUiType(os.path.join(os.path.dirname(__file__), 'shortestpathbottombar.ui'))

class ShortestPathBottomBar(KadasBottomBar, WIDGET):

    def __init__(self, canvas, action):
        KadasBottomBar.__init__(self, canvas, "orange")
        self.setupUi(self)
        self.setStyleSheet("QFrame { background-color: orange; }")
        self.action = action

        # Config buttons (icons, actions, disable)
        # Icons
        self.btnGPSOrigin.setIcon(icon("gps.png"))
        self.btnGPSDestination.setIcon(icon("gps.png"))
        self.btnGPSWaypoints.setIcon(icon("gps.png"))

        self.btnMapToolOrigin.setIcon(QIcon(":/kadas/icons/pick"))
        self.btnMapToolDestination.setIcon(QIcon(":/kadas/icons/pick"))
        self.btnMapToolWaypoints.setIcon(QIcon(":/kadas/icons/pick"))

        self.btnAddWaypoints.setIcon(QIcon(":/kadas/icons/add"))

        self.btnClose.setIcon(QIcon(":/kadas/icons/close"))

        # Disable GPS buttons for now
        self.btnGPSOrigin.setEnabled(False)
        self.btnGPSDestination.setEnabled(False)
        self.btnGPSWaypoints.setEnabled(False)

        # Tooltips
        gps_tooltip = 'Get GPS location'
        self.btnGPSOrigin.setToolTip(gps_tooltip)
        self.btnGPSDestination.setToolTip(gps_tooltip)
        self.btnGPSWaypoints.setToolTip(gps_tooltip)

        pick_tooltip = 'Choose location on the map'
        self.btnMapToolOrigin.setToolTip(pick_tooltip)
        self.btnMapToolDestination.setToolTip(pick_tooltip)
        self.btnMapToolWaypoints.setToolTip(pick_tooltip)

        self.btnAddWaypoints.setToolTip('Add waypoint')

        self.btnClose.setToolTip('CLose routing dialog')

        # Connections
        self.btnClose.clicked.connect(self.action.toggle)

        # Search boxes
        self.originSearchBox = KadasSearchBox(canvas)
        self.originSearchBox.init(canvas)
        self.originSearchBox.addSearchProvider(KadasCoordinateSearchProvider(canvas))
        self.originSearchBox.addSearchProvider(KadasLocationSearchProvider(canvas))
        self.originSearchBox.addSearchProvider(KadasLocalDataSearchProvider(canvas))
        self.originSearchBox.addSearchProvider(KadasPinSearchProvider(canvas))
        self.originSearchBox.addSearchProvider(KadasRemoteDataSearchProvider(canvas))
        self.originSearchBox.addSearchProvider(KadasWorldLocationSearchProvider(canvas))
        self.layout().addWidget(self.originSearchBox, 0, 1)

        self.destinationSearchBox = KadasSearchBox(canvas)
        self.destinationSearchBox.init(canvas)
        self.destinationSearchBox.addSearchProvider(KadasCoordinateSearchProvider(canvas))
        self.destinationSearchBox.addSearchProvider(KadasLocationSearchProvider(canvas))
        self.destinationSearchBox.addSearchProvider(KadasLocalDataSearchProvider(canvas))
        self.destinationSearchBox.addSearchProvider(KadasPinSearchProvider(canvas))
        self.destinationSearchBox.addSearchProvider(KadasRemoteDataSearchProvider(canvas))
        self.destinationSearchBox.addSearchProvider(KadasWorldLocationSearchProvider(canvas))
        self.layout().addWidget(self.destinationSearchBox, 1, 1)

        self.waypointsSearchBox = KadasSearchBox(canvas)
        self.waypointsSearchBox.init(canvas)
        self.waypointsSearchBox.addSearchProvider(KadasCoordinateSearchProvider(canvas))
        self.waypointsSearchBox.addSearchProvider(KadasLocationSearchProvider(canvas))
        self.waypointsSearchBox.addSearchProvider(KadasLocalDataSearchProvider(canvas))
        self.waypointsSearchBox.addSearchProvider(KadasPinSearchProvider(canvas))
        self.waypointsSearchBox.addSearchProvider(KadasRemoteDataSearchProvider(canvas))
        self.waypointsSearchBox.addSearchProvider(KadasWorldLocationSearchProvider(canvas))
        self.layout().addWidget(self.waypointsSearchBox, 2, 1)
