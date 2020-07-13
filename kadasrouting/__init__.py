# -*- coding: utf-8 -*-

def classFactory(iface):
    from .plugin import RoutingPlugin
    return RoutingPlugin(iface)
