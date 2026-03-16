# -*- coding: utf-8 -*-
"""
kadas.utils
-----------
Kadas utility module, providing a properly cast KadasPluginInterface instance.

Usage in plugins:
    from kadas.utils import iface
"""

import qgis.utils as _qgis_utils

# Module-level iface, populated by initInterface() called from C++ on startup.
# Will be a KadasPluginInterface instance (or None before initialization).
iface = None


def initInterface(pointer):
    """
    Initialize the kadas iface object from a raw C++ pointer.
    Called by KadasPythonIntegration::initPython() after qgis.utils.initInterface().
    """
    from kadas.kadasgui import KadasPluginInterface

    # Let qgis.utils wrap the pointer first as QgisInterface
    _qgis_utils.initInterface(pointer)

    # Then cast to the more specific KadasPluginInterface
    global iface
    iface = KadasPluginInterface.cast(_qgis_utils.iface)