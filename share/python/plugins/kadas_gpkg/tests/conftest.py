import os
import sys
from unittest import mock

import pytest

# On Windows, kadas/QGIS DLLs must be added to the DLL search path before any
# QGIS imports.  Set the KADAS_BIN_DIR environment variable to the kadas bin
# directory (e.g. build/output/bin) when running tests.
if sys.platform == "win32":
    kadas_bin = os.environ.get("KADAS_BIN_DIR", "")
    if kadas_bin:
        os.add_dll_directory(kadas_bin)
    # qgis._gui has a static link dependency on qgis_3d.dll (Qgs3DMapCanvas is
    # used in the QgisInterface base class).  On Windows the DLL loader must
    # resolve qgis_3d.dll before qgis._gui is loaded; importing qgis._3d first
    # ensures this and avoids "DLL load failed: The specified procedure could
    # not be found" when importing qgis.gui.
    import qgis._3d  # noqa: F401

from qgis.gui import QgsMapCanvas  # noqa: E402
from qgis.PyQt.QtCore import QSize  # noqa: E402
from qgis.PyQt.QtWidgets import QMainWindow  # noqa: E402
from qgis.testing import start_app  # noqa: E402


@pytest.fixture(scope="session", autouse=True)
def qgs_app():
    """Start QgsApplication once for the whole test session.
    Note: no explicit exitQgis() here - start_app() registers an atexit handler
    for cleanup, and calling exitQgis() manually causes a segfault on teardown.
    """
    yield start_app()


@pytest.fixture(scope="session")
def iface():
    """
    Return a mock KadasPluginInterface with a real QgsMapCanvas wired in.
    Follows the same pattern as qgis.testing.mocked.get_iface() but uses
    KadasPluginInterface as the spec so Kadas-specific methods are available.
    """
    # Imported here (not at module level) so that loading the kadas C extension
    # does not interfere with QgsApplication initialisation in qgs_app.
    from kadas.kadasgui import KadasPluginInterface  # noqa: PLC0415

    my_iface = mock.Mock(spec=KadasPluginInterface)

    main_window = QMainWindow()
    my_iface.mainWindow.return_value = main_window

    canvas = QgsMapCanvas(main_window)
    canvas.resize(QSize(400, 400))
    my_iface.mapCanvas.return_value = canvas

    return my_iface
