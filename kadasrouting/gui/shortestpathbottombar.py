import os
from PyQt5 import uic
from PyQt5.QtGui import QIcon
from kadasrouting.gui.routingsearchbox import addSearchBox

from kadas.kadasgui import KadasBottomBar

WIDGET, BASE = uic.loadUiType(os.path.join(os.path.dirname(__file__), 'shortestpathbottombar.ui'))

class ShortestPathBottomBar(KadasBottomBar, WIDGET):

    def __init__(self, canvas, action):
        KadasBottomBar.__init__(self, canvas, "orange")        
        self.setupUi(self)
        self.setStyleSheet("QFrame { background-color: orange; }")        
        self.action = action
        self.btnClose.setIcon(QIcon(":/kadas/icons/close"))
        self.btnClose.clicked.connect(self.action.toggle)
        self.originSearchBox = addSearchBox(self.originWidget)

        