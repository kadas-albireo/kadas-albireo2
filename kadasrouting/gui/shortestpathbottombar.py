import os
from PyQt5 import uic
from PyQt5.QtGui import QIcon

from kadas.kadasgui import KadasBottomBar
from kadasrouting.gui.routingsearchbox import RoutingSearchBox

WIDGET, BASE = uic.loadUiType(os.path.join(os.path.dirname(__file__), 'shortestpathbottombar.ui'))

class ShortestPathBottomBar(KadasBottomBar, WIDGET):

    def __init__(self, canvas, action):
        KadasBottomBar.__init__(self, canvas, "orange")
        self.setupUi(self)
        self.setStyleSheet("QFrame { background-color: orange; }")
        self.action = action
        self.canvas = canvas

        self.btnAddWaypoints.setIcon(QIcon(":/kadas/icons/add"))
        self.btnClose.setIcon(QIcon(":/kadas/icons/close"))
        self.btnAddWaypoints.setToolTip('Add waypoint')
        self.btnClose.setToolTip('Close routing dialog')

        # Connections
        self.btnClose.clicked.connect(self.action.toggle)

        # Search boxes
        self.originSearchBox = RoutingSearchBox(canvas)
        self.layout().addWidget(self.originSearchBox, 0, 1)

        self.destinationSearchBox = RoutingSearchBox(canvas)
        self.layout().addWidget(self.destinationSearchBox, 1, 1)

        self.waypointsSearchBox = RoutingSearchBox(canvas)
        self.layout().addWidget(self.waypointsSearchBox, 2, 1)



        