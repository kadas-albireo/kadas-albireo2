# -*- coding: utf-8 -*-
import os
from PyQt5.QtGui import QIcon


def iconPath(name):
    return os.path.join(os.path.dirname(__file__), "icons", name)

def icon(name):
    return QIcon(iconPath(name))
