from kadas_gpkg.kadas_gpkg_export_dialog import KadasGpkgExportDialog


def test_export_dialog_instantiation(qgs_app, iface):
    """Smoke test: KadasGpkgExportDialog can be constructed without errors."""
    dialog = KadasGpkgExportDialog(None, iface)

    assert dialog is not None
    dialog.close()
