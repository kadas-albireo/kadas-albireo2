import os
import sys
from unittest import mock

import pytest

# On Windows, kadas/QGIS DLLs must be added to the DLL search path before any
# QGIS imports.  Set the KADAS_BIN_DIR environment variable to the kadas bin
# directory (e.g. build/output/bin) when running tests.
if sys.platform == "win32":
    import ctypes

    kadas_bin = os.environ.get("KADAS_BIN_DIR", "")
    if kadas_bin:
        os.add_dll_directory(kadas_bin)
    # qgis_gui.dll has a load-time dependency on qgis_3d.dll (Qgs3DMapCanvas
    # is referenced in the QgisInterface base class).  Without pre-loading the
    # native DLL, Windows fails to initialise _gui_p.pyd with "The specified
    # procedure could not be found" (ERROR_PROC_NOT_FOUND).  We load it via
    # ctypes — which only invokes LoadLibraryEx on the native DLL, never the
    # Python binding init code in _3d_p.pyd — to stay robust against version
    # mismatches in the Python binding layer while still satisfying the
    # load-time linker requirement.
    ctypes.CDLL("qgis_3d.dll")

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
