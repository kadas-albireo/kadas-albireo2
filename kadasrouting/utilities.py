# -*- coding: utf-8 -*-
import os
from PyQt5.QtGui import QIcon
from PyQt5.QtWidgets import QMessageBox
from qgis.utils import iface
from qgis.core import Qgis


def iconPath(name):
    return os.path.join(os.path.dirname(__file__), "icons", name)

def icon(name):
    return QIcon(iconPath(name))

def showMessageBox(text):
    QMessageBox.information(iface.mainWindow(),  'Log', text)

def pushMessage(text):
    iface.messageBar().pushMessage("Log", text, level=Qgis.Info)