# -*- coding: utf-8 -*-
import os
from PyQt5.QtGui import QIcon

def icon(name):
    return QIcon(os.path.join(os.path.dirname(__file__), "icons", name))
