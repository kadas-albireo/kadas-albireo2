"""Tests for KadasGpkgLayersList (QTreeWidget-based layer selection widget).

The tests build a small project layer tree backed by real GeoPackage files
(OGR provider, which IS in SUPPORTED_PROVIDERS) and verify the selection /
deselection logic including group tristate behaviour.

Structure used in most tests:

    root
    ├── GroupA
    │   ├── layer_a1   (ogr, small → Checked by default)
    │   └── layer_a2   (ogr, small → Checked by default)
    └── GroupB
        ├── layer_b1   (ogr, small → Checked by default)
        └── layer_b2   (ogr, small → Checked by default)
"""

import pytest
from kadas_gpkg.kadas_gpkg_layer_list import (
    _LAYER_ID_ROLE,
    KadasGpkgImportLayersList,
    KadasGpkgLayersList,
)
from qgis.core import (
    QgsCoordinateTransformContext,
    QgsProject,
    QgsVectorFileWriter,
    QgsVectorLayer,
)
from qgis.PyQt.QtCore import Qt

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------


def _make_ogr_layer(name: str, gpkg_path: str) -> QgsVectorLayer:
    """Write a tiny point GeoPackage and return an OGR-backed QgsVectorLayer."""
    mem = QgsVectorLayer("Point?crs=EPSG:4326&field=id:integer", name, "memory")
    assert mem.isValid()
    opts = QgsVectorFileWriter.SaveVectorOptions()
    opts.driverName = "GPKG"
    opts.layerName = name
    QgsVectorFileWriter.writeAsVectorFormatV3(
        mem, gpkg_path, QgsCoordinateTransformContext(), opts
    )
    layer = QgsVectorLayer(gpkg_path + f"|layername={name}", name, "ogr")
    assert layer.isValid(), f"Could not create OGR layer '{name}' at {gpkg_path}"
    return layer


def _find_item(widget: KadasGpkgLayersList, layer_id: str):
    """Return the QTreeWidgetItem for *layer_id*, or None."""
    return _find_item_in(widget._tree.invisibleRootItem(), layer_id)


def _find_item_in(parent, layer_id: str):
    for i in range(parent.childCount()):
        child = parent.child(i)
        if child.data(0, _LAYER_ID_ROLE) == layer_id:
            return child
        found = _find_item_in(child, layer_id)
        if found is not None:
            return found
    return None


def _find_group_item(widget: KadasGpkgLayersList, group_name: str):
    """Return the QTreeWidgetItem for the group with *group_name*, or None."""
    return _find_group_in(widget._tree.invisibleRootItem(), group_name)


def _find_group_in(parent, group_name: str):
    for i in range(parent.childCount()):
        child = parent.child(i)
        if child.data(0, _LAYER_ID_ROLE) is None and child.text(0) == group_name:
            return child
        found = _find_group_in(child, group_name)
        if found is not None:
            return found
    return None


# ---------------------------------------------------------------------------
# Fixtures
# ---------------------------------------------------------------------------


@pytest.fixture(autouse=True)
def clean_project():
    """Ensure each test starts with an empty QgsProject."""
    QgsProject.instance().clear()
    yield
    QgsProject.instance().clear()


@pytest.fixture
def four_layer_project(tmp_path):
    """Build the two-group, four-layer project tree described in the module docstring.

    Layers are backed by real GeoPackage files (OGR provider) so they pass the
    SUPPORTED_PROVIDERS filter inside KadasGpkgLayersList.

    Returns (widget, layer_a1, layer_a2, layer_b1, layer_b2).
    """
    project = QgsProject.instance()
    root = project.layerTreeRoot()

    layer_a1 = _make_ogr_layer("LayerA1", str(tmp_path / "a1.gpkg"))
    layer_a2 = _make_ogr_layer("LayerA2", str(tmp_path / "a2.gpkg"))
    layer_b1 = _make_ogr_layer("LayerB1", str(tmp_path / "b1.gpkg"))
    layer_b2 = _make_ogr_layer("LayerB2", str(tmp_path / "b2.gpkg"))

    project.addMapLayer(layer_a1, False)
    project.addMapLayer(layer_a2, False)
    project.addMapLayer(layer_b1, False)
    project.addMapLayer(layer_b2, False)

    group_a = root.addGroup("GroupA")
    group_a.addLayer(layer_a1)
    group_a.addLayer(layer_a2)

    group_b = root.addGroup("GroupB")
    group_b.addLayer(layer_b1)
    group_b.addLayer(layer_b2)

    widget = KadasGpkgLayersList()
    return widget, layer_a1, layer_a2, layer_b1, layer_b2


# ---------------------------------------------------------------------------
# Basic construction
# ---------------------------------------------------------------------------


class TestConstruction:
    def test_widget_created(self, four_layer_project):
        widget, *_ = four_layer_project
        assert widget is not None

    def test_all_layers_present(self, four_layer_project):
        widget, la1, la2, lb1, lb2 = four_layer_project
        for layer in (la1, la2, lb1, lb2):
            assert (
                _find_item(widget, layer.id()) is not None
            ), f"Layer '{layer.name()}' not found in widget"

    def test_groups_present(self, four_layer_project):
        widget, *_ = four_layer_project
        assert _find_group_item(widget, "GroupA") is not None
        assert _find_group_item(widget, "GroupB") is not None

    def test_all_layers_checked_by_default(self, four_layer_project):
        """Memory layers have size 0, so they are all checked by default."""
        widget, la1, la2, lb1, lb2 = four_layer_project
        for layer in (la1, la2, lb1, lb2):
            item = _find_item(widget, layer.id())
            assert (
                item.checkState(0) == Qt.CheckState.Checked
            ), f"Layer '{layer.name()}' should be Checked by default"

    def test_groups_checked_by_default(self, four_layer_project):
        widget, *_ = four_layer_project
        for name in ("GroupA", "GroupB"):
            grp = _find_group_item(widget, name)
            assert (
                grp.checkState(0) == Qt.CheckState.Checked
            ), f"Group '{name}' should be Checked when all children are Checked"


# ---------------------------------------------------------------------------
# getSelectedLayers
# ---------------------------------------------------------------------------


class TestGetSelectedLayers:
    def test_all_selected_initially(self, four_layer_project):
        widget, la1, la2, lb1, lb2 = four_layer_project
        selected = widget.getSelectedLayers()
        assert set(selected.keys()) == {la1.id(), la2.id(), lb1.id(), lb2.id()}

    def test_uncheck_single_layer(self, four_layer_project):
        widget, la1, la2, lb1, lb2 = four_layer_project
        item = _find_item(widget, la1.id())
        item.setCheckState(0, Qt.CheckState.Unchecked)
        selected = widget.getSelectedLayers()
        assert la1.id() not in selected
        assert {la2.id(), lb1.id(), lb2.id()} <= selected.keys()

    def test_uncheck_all_layers(self, four_layer_project):
        widget, la1, la2, lb1, lb2 = four_layer_project
        for layer in (la1, la2, lb1, lb2):
            _find_item(widget, layer.id()).setCheckState(0, Qt.CheckState.Unchecked)
        assert widget.getSelectedLayers() == {}

    def test_deselect_all_button(self, four_layer_project):
        widget, *_ = four_layer_project
        widget._deselect_all()
        assert widget.getSelectedLayers() == {}

    def test_select_all_button_after_deselect(self, four_layer_project):
        widget, la1, la2, lb1, lb2 = four_layer_project
        widget._deselect_all()
        widget._select_all()
        selected = widget.getSelectedLayers()
        assert set(selected.keys()) == {la1.id(), la2.id(), lb1.id(), lb2.id()}


# ---------------------------------------------------------------------------
# Group tristate – upward sync (child → parent, handled by ItemIsAutoTristate)
# ---------------------------------------------------------------------------


class TestGroupTristateUpward:
    def test_uncheck_one_child_makes_group_partial(self, four_layer_project):
        widget, la1, la2, *_ = four_layer_project
        _find_item(widget, la1.id()).setCheckState(0, Qt.CheckState.Unchecked)
        grp = _find_group_item(widget, "GroupA")
        assert grp.checkState(0) == Qt.CheckState.PartiallyChecked

    def test_uncheck_all_children_makes_group_unchecked(self, four_layer_project):
        widget, la1, la2, *_ = four_layer_project
        _find_item(widget, la1.id()).setCheckState(0, Qt.CheckState.Unchecked)
        _find_item(widget, la2.id()).setCheckState(0, Qt.CheckState.Unchecked)
        grp = _find_group_item(widget, "GroupA")
        assert grp.checkState(0) == Qt.CheckState.Unchecked

    def test_recheck_all_children_makes_group_checked(self, four_layer_project):
        widget, la1, la2, *_ = four_layer_project
        _find_item(widget, la1.id()).setCheckState(0, Qt.CheckState.Unchecked)
        _find_item(widget, la2.id()).setCheckState(0, Qt.CheckState.Unchecked)
        _find_item(widget, la1.id()).setCheckState(0, Qt.CheckState.Checked)
        _find_item(widget, la2.id()).setCheckState(0, Qt.CheckState.Checked)
        grp = _find_group_item(widget, "GroupA")
        assert grp.checkState(0) == Qt.CheckState.Checked

    def test_other_group_unaffected(self, four_layer_project):
        """Unchecking a layer in GroupA must not change GroupB's state."""
        widget, la1, *rest = four_layer_project
        _find_item(widget, la1.id()).setCheckState(0, Qt.CheckState.Unchecked)
        grp_b = _find_group_item(widget, "GroupB")
        assert grp_b.checkState(0) == Qt.CheckState.Checked


# ---------------------------------------------------------------------------
# Group tristate – downward propagation (parent → children)
# ---------------------------------------------------------------------------


class TestGroupTristateDownward:
    def test_uncheck_group_unchecks_all_children(self, four_layer_project):
        widget, la1, la2, *_ = four_layer_project
        grp = _find_group_item(widget, "GroupA")
        grp.setCheckState(0, Qt.CheckState.Unchecked)
        assert _find_item(widget, la1.id()).checkState(0) == Qt.CheckState.Unchecked
        assert _find_item(widget, la2.id()).checkState(0) == Qt.CheckState.Unchecked

    def test_check_group_checks_all_children(self, four_layer_project):
        widget, la1, la2, *_ = four_layer_project
        # First uncheck everything, then re-check the group
        _find_item(widget, la1.id()).setCheckState(0, Qt.CheckState.Unchecked)
        _find_item(widget, la2.id()).setCheckState(0, Qt.CheckState.Unchecked)
        grp = _find_group_item(widget, "GroupA")
        grp.setCheckState(0, Qt.CheckState.Checked)
        assert _find_item(widget, la1.id()).checkState(0) == Qt.CheckState.Checked
        assert _find_item(widget, la2.id()).checkState(0) == Qt.CheckState.Checked

    def test_partial_group_children_unchanged(self, four_layer_project):
        """When auto-tristate sets a group to PartiallyChecked its children must
        not be altered — PartiallyChecked is a read-only computed state.
        """
        widget, la1, la2, *_ = four_layer_project
        _find_item(widget, la1.id()).setCheckState(0, Qt.CheckState.Unchecked)
        grp = _find_group_item(widget, "GroupA")
        assert grp.checkState(0) == Qt.CheckState.PartiallyChecked
        # Children must keep their individual states
        assert _find_item(widget, la1.id()).checkState(0) == Qt.CheckState.Unchecked
        assert _find_item(widget, la2.id()).checkState(0) == Qt.CheckState.Checked

    def test_uncheck_group_does_not_affect_sibling_group(self, four_layer_project):
        widget, la1, la2, lb1, lb2 = four_layer_project
        grp_a = _find_group_item(widget, "GroupA")
        grp_a.setCheckState(0, Qt.CheckState.Unchecked)
        # GroupB and its children must be untouched
        assert _find_item(widget, lb1.id()).checkState(0) == Qt.CheckState.Checked
        assert _find_item(widget, lb2.id()).checkState(0) == Qt.CheckState.Checked


# ---------------------------------------------------------------------------
# updateLayerList – in-GPKG layers become disabled
# ---------------------------------------------------------------------------


class TestUpdateLayerList:
    def test_in_gpkg_layer_becomes_disabled(self, four_layer_project):
        """A layer whose source path starts with the target gpkg path is disabled."""
        widget, la1, la2, lb1, lb2 = four_layer_project
        # la1's source already IS a gpkg file — use that path as the "target"
        target_gpkg = la1.source().split("|")[0]
        widget.updateLayerList(target_gpkg)
        item = _find_item(widget, la1.id())
        assert not (
            item.flags() & Qt.ItemFlag.ItemIsEnabled
        ), "Layer whose source is in the target gpkg must be disabled"

    def test_in_gpkg_layer_excluded_from_selected(self, four_layer_project):
        widget, la1, la2, lb1, lb2 = four_layer_project
        target_gpkg = la1.source().split("|")[0]
        widget.updateLayerList(target_gpkg)
        selected = widget.getSelectedLayers()
        assert la1.id() not in selected

    def test_non_gpkg_layer_stays_enabled(self, four_layer_project):
        widget, la1, la2, lb1, lb2 = four_layer_project
        target_gpkg = la1.source().split("|")[0]
        widget.updateLayerList(target_gpkg)
        item = _find_item(widget, la2.id())
        assert item.flags() & Qt.ItemFlag.ItemIsEnabled


# ---------------------------------------------------------------------------
# Unsupported provider layers are hidden
# ---------------------------------------------------------------------------


class TestUnsupportedProviders:
    def test_unsupported_provider_layer_not_shown(self, qgs_app):
        """A layer whose provider is not in SUPPORTED_PROVIDERS must be omitted."""
        project = QgsProject.instance()
        root = project.layerTreeRoot()

        # "memory" is not in SUPPORTED_PROVIDERS, so it should be filtered out
        layer = QgsVectorLayer("Point?crs=EPSG:4326", "MemOnly", "memory")
        assert layer.isValid()
        assert layer.dataProvider().name() == "memory"

        project.addMapLayer(layer, False)
        root.addLayer(layer)

        widget = KadasGpkgLayersList()
        assert (
            _find_item(widget, layer.id()) is None
        ), "A 'memory' provider layer should be filtered out of the widget"

    def test_ogr_layer_is_shown(self, qgs_app, tmp_path):
        """A small real OGR layer must appear in the widget."""
        layer = _make_ogr_layer("OgrLayer", str(tmp_path / "shown.gpkg"))

        project = QgsProject.instance()
        project.addMapLayer(layer, False)
        project.layerTreeRoot().addLayer(layer)

        widget = KadasGpkgLayersList()
        assert (
            _find_item(widget, layer.id()) is not None
        ), "An OGR layer should appear in the widget"


# ---------------------------------------------------------------------------
# Regression: _select_all() must check group items on the very first call
# ---------------------------------------------------------------------------


class TestSelectAllGroupsRegression:
    def test_group_datachanged_fires_after_children_are_checked(self, four_layer_project):
        """The group's model dataChanged must fire only after all its children are
        already in the target state.

        Root cause (now fixed): the old _set_subtree_check_state set the group
        item first and then recursed into its children. Qt's ItemIsAutoTristate
        recalculates the group upward via model signals; with blockSignals active
        that upward path was suppressed, so the group's dataChanged notification
        fired while children were still in the old mixed state — causing a stale
        repaint with a PartiallyChecked indicator.

        Current implementation: _set_leaves_check_state only touches enabled leaf
        items. Qt's ItemIsAutoTristate recalculates group states upward after each
        leaf change, so by the time the group's dataChanged fires all children are
        already in the target state.
        """
        widget, la1, la2, lb1, lb2 = four_layer_project

        # Put GroupA into a PartiallyChecked state
        _find_item(widget, la2.id()).setCheckState(0, Qt.CheckState.Unchecked)
        grp_a = _find_group_item(widget, "GroupA")
        assert grp_a.checkState(0) == Qt.CheckState.PartiallyChecked, \
            "Precondition: GroupA must be PartiallyChecked"

        # Capture the state of la2 AT THE MOMENT the group's dataChanged fires.
        # With the buggy order (group set before children), dataChanged for the
        # group fires while la2 is still Unchecked — the view would render the
        # group as PartiallyChecked.
        # With the fix (children set first), dataChanged fires after la2 is
        # already Checked — the view renders Checked correctly.
        la2_states_when_group_fires = []

        def on_data_changed(tl, br, roles):
            item = widget._tree.itemFromIndex(tl)
            if item is grp_a:
                la2_item = _find_item(widget, la2.id())
                la2_states_when_group_fires.append(la2_item.checkState(0))

        widget._tree.model().dataChanged.connect(on_data_changed)

        widget._select_all()

        # The group's dataChanged must have fired
        assert la2_states_when_group_fires, \
            "dataChanged was never emitted for the group during _select_all()"

        # When it fired, la2 must already have been Checked.
        # If la2 was still Unchecked at that moment, the visual repaint would
        # show the group as PartiallyChecked — that is the bug.
        assert la2_states_when_group_fires[-1] == Qt.CheckState.Checked, (
            f"la2 was {la2_states_when_group_fires[-1]!r} when the group's "
            f"dataChanged fired — the view would render GroupA as PartiallyChecked "
            f"instead of Checked"
        )


# ---------------------------------------------------------------------------
# KadasGpkgImportLayersList – already-loaded layer detection
# ---------------------------------------------------------------------------


def _make_import_xml(layers, tree_ids):
    """Build a minimal QGIS project XML string for use with loadFromGpkg.

    *layers* is a list of (layer_id, layer_name, layer_type) tuples.
    *tree_ids* is the ordered list of layer IDs to place in the layer-tree.
    """
    maplayer_blocks = "\n".join(
        f"  <maplayer type=\"{ltype}\">\n"
        f"    <id>{lid}</id>\n"
        f"    <layername>{lname}</layername>\n"
        f"    <provider>ogr</provider>\n"
        f"  </maplayer>"
        for lid, lname, ltype in layers
    )
    tree_nodes = "\n".join(
        f'    <layer-tree-layer id="{lid}" name="{lname}"/>'
        for lid, lname, _ in layers
        if lid in tree_ids
    )
    return (
        "<qgis>\n"
        + maplayer_blocks
        + "\n  <layer-tree-group name=\"root\">\n"
        + tree_nodes
        + "\n  </layer-tree-group>\n</qgis>"
    )


class TestImportAlreadyLoaded:
    """Tests for already-loaded layer detection in KadasGpkgImportLayersList."""

    @pytest.fixture
    def import_setup(self, tmp_path):
        """Create an OGR layer loaded into QgsProject from a GPKG, plus an import
        widget primed with XML describing that layer and a second unloaded layer.

        Returns (widget, gpkg_path, loaded_id, unloaded_id).
        """
        gpkg_path = str(tmp_path / "test.gpkg")
        loaded = _make_ogr_layer("LoadedLayer", gpkg_path)
        loaded_id = loaded.id()

        project = QgsProject.instance()
        project.addMapLayer(loaded, False)
        project.layerTreeRoot().addLayer(loaded)

        # A second layer ID that is NOT loaded in the project
        unloaded_id = "unloaded_layer_id_00000000"

        xml = _make_import_xml(
            [
                (loaded_id, "LoadedLayer", "vector"),
                (unloaded_id, "UnloadedLayer", "vector"),
            ],
            [loaded_id, unloaded_id],
        )

        widget = KadasGpkgImportLayersList()
        widget.loadFromGpkg(gpkg_path, xml)
        return widget, gpkg_path, loaded_id, unloaded_id

    def test_already_loaded_layer_is_disabled(self, import_setup):
        """A layer whose source starts with the GPKG path must be disabled."""
        widget, gpkg_path, loaded_id, unloaded_id = import_setup
        item = _find_item(widget, loaded_id)
        assert item is not None
        assert not (item.flags() & Qt.ItemFlag.ItemIsEnabled), (
            "Already-loaded layer must be disabled in the import list"
        )

    def test_already_loaded_layer_excluded_from_results(self, import_setup):
        """Already-loaded layers must not appear in getSelectedLayerIds()."""
        widget, gpkg_path, loaded_id, unloaded_id = import_setup
        result = widget.getSelectedLayerIds()
        assert loaded_id not in result

    def test_unloaded_layer_is_enabled(self, import_setup):
        """A layer that is not yet in the project must be enabled for selection."""
        widget, gpkg_path, loaded_id, unloaded_id = import_setup
        item = _find_item(widget, unloaded_id)
        assert item is not None
        assert item.flags() & Qt.ItemFlag.ItemIsEnabled, (
            "Layer not loaded from this GPKG must be enabled"
        )

    def test_unloaded_layer_appears_in_results_when_checked(self, import_setup):
        """A checked unloaded layer must appear in getSelectedLayerIds()."""
        widget, gpkg_path, loaded_id, unloaded_id = import_setup
        item = _find_item(widget, unloaded_id)
        item.setCheckState(0, Qt.CheckState.Checked)
        result = widget.getSelectedLayerIds()
        assert unloaded_id in result
