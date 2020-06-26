# -*- coding: utf-8 -*-

import sys
import os
import site

site.addsitedir(os.path.abspath(os.path.dirname(__file__) + '/libs/qgisvalhalla'))

def classFactory(iface):
    from .plugin import RoutingPlugin
    return RoutingPlugin(iface)
