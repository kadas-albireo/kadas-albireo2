import os
from PyQt5 import uic
from PyQt5.QtGui import QIcon
from kadas.kadasgui import KadasBottomBar

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

        # Connections
        self.btnClose.clicked.connect(self.action.toggle)

        self.originSearchBox = addSearchBox(self.originWidget)

