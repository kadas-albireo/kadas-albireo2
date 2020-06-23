from PyQt5.QtWidgets import QWidget, QHBoxLayout, QLineEdit 

from kadas.kadasgui import (
    KadasBottomBar, 
    KadasSearchBox, 
    KadasCoordinateSearchProvider, 
    KadasLocationSearchProvider, 
    KadasLocalDataSearchProvider,
    KadasRemoteDataSearchProvider,
    KadasWorldLocationSearchProvider)

def WrongPlaceException(Exception):
    pass

class RoutingSearchBox(QWidget):
    
    def __init__(self):
        QWidget.__init__(self)
        self.layout = QHBoxLayout()
        self.text = QLineEdit()

        '''
        This doesnt work and crashes KADAS, but we should explore this option
        self.originSearchBox.init(canvas)
        self.originSearchBox.addSearchProvider(KadasCoordinateSearchProvider(canvas))
        self.originSearchBox.addSearchProvider(KadasLocationSearchProvider(canvas))
        self.originSearchBox.addSearchProvider(KadasLocalDataSearchProvider(canvas))
        self.originSearchBox.addSearchProvider(KadasPinSearchProvider(canvas))
        self.originSearchBox.addSearchProvider(KadasRemoteDataSearchProvider(canvas))
        self.originSearchBox.addSearchProvider(KadasWorldLocationSearchProvider(canvas))
        '''

        self.layout.addWidget(self.text)
        self.setLayout(self.layout)

        #TODO add buttons

    def coords(self):
        #geocode and return coordinates based on text in the text field, or raise WrongPlaceException
        return 0,0


def addSearchBox(containerWidget):
    layout = QHBoxLayout()
    layout.setMargin(0)
    box = RoutingSearchBox()
    layout.addWidget(box)
    containerWidget.setLayout(layout)
    return box
    