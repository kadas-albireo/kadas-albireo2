import os
import json

from qgis.PyQt import uic

from qgis.PyQt.QtCore import QSettings

from kadasrouting.utilities import localeName

WIDGET, BASE = uic.loadUiType(os.path.join(os.path.dirname(__file__), "disclaimerdialog.ui"))

class DisclaimerDialog(BASE, WIDGET):

    def __init__(self, parent=None):    	
        super(DisclaimerDialog, self).__init__(parent)
        self.setupUi(self)
        indexes = ["en", "de", "fr", "it"]
        try:
        	index = indexes.index(localeName())
        except ValueError:
        	index = 0 
        self.stackedWidget.setCurrentIndex(index)
        self.btnClose.clicked.connect(self.closeClicked)

    def closeClicked(self):
        QSettings().setValue("kadasrouting/showDisclaimer", not self.checkBox.isChecked())
        self.close()
