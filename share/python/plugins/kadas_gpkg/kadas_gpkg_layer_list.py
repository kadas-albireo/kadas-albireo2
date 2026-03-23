import os
import re

from qgis.core import QgsIconUtils, QgsLayerTree, QgsLayerTreeModel, QgsProject
from qgis.PyQt.QtCore import Qt
from qgis.PyQt.QtGui import QIcon
from qgis.PyQt.QtWidgets import (
    QHBoxLayout,
    QPushButton,
    QSizePolicy,
    QTreeWidget,
    QTreeWidgetItem,
    QVBoxLayout,
    QWidget,
)

# Role used to store the layer id on leaf items
_LAYER_ID_ROLE = Qt.ItemDataRole.UserRole + 1



class KadasGpkgLayersList(QWidget):
    """Layer selection widget mirroring the project layer tree structure.

    Displays layers grouped and ordered exactly as in the Kadas main window.
    Group checkboxes support tristate (checked / partial / unchecked).
    A second column shows a warning icon for large/unknown-size layers,
    or a success icon for layers already present in the target GeoPackage.

    Public API (unchanged from original):
        updateLayerList(existingOutputGpkg)
        getSelectedLayers()              -> {layerid: layerType}
        getUnselectedProjectLayers()     -> {layerid: QgsMapLayer}
    """

    WARN_SIZE = 50 * 1024 * 1024
    PROJECT_LAYER_PROVIDERS = ["bullseye", "guide_grid", "KadasItemLayer", "annotation"]
    SUPPORTED_PROVIDERS = [
        "delimitedtext",
        "gdal",
        "gpx",
        "mssql",
        "ogr",
        "postgres",
        "spatialite",
        "wcs",
        "wms",
        "WFS",
        "arcgisfeatureserver",
    ]

    def __init__(self, parent=None):
        QWidget.__init__(self, parent)

        self._icon_warn = QIcon(":/images/themes/default/mIconWarning.svg")
        self._icon_ok   = QIcon(":/images/themes/default/mIconSuccess.svg")
        self._icon_none = QIcon()

        # layer-ids already stored in the target gpkg
        self._in_gpkg_ids = set()

        # Two-column tree: col 0 = checkbox + layer icon + name,  col 1 = status icon
        self._tree = QTreeWidget(self)
        self._tree.setColumnCount(2)
        self._tree.setHeaderHidden(True)
        self._tree.setSizePolicy(QSizePolicy.Policy.Expanding, QSizePolicy.Policy.Expanding)
        self._tree.header().setStretchLastSection(False)
        self._tree.header().setSectionResizeMode(0, self._tree.header().ResizeMode.Stretch)
        self._tree.header().setSectionResizeMode(1, self._tree.header().ResizeMode.Fixed)
        self._tree.setColumnWidth(1, 22)

        # Populate from the live project layer tree
        root = QgsProject.instance().layerTreeRoot()
        self._build_tree(root, self._tree.invisibleRootItem())
        self._tree.expandAll()

        # itemChanged drives tristate propagation (group -> children)
        self._tree.itemChanged.connect(self._on_item_changed)

        # Buttons
        btn_all  = QPushButton(self.tr("Select All"))
        btn_none = QPushButton(self.tr("Deselect All"))
        btn_all.clicked.connect(self._select_all)
        btn_none.clicked.connect(self._deselect_all)

        btn_layout = QHBoxLayout()
        btn_layout.setContentsMargins(0, 0, 0, 0)
        btn_layout.addWidget(btn_all)
        btn_layout.addWidget(btn_none)
        btn_layout.addStretch()

        layout = QVBoxLayout()
        layout.setContentsMargins(0, 0, 0, 0)
        layout.addWidget(self._tree)
        layout.addLayout(btn_layout)
        self.setLayout(layout)

    # ------------------------------------------------------------------
    # Public API
    # ------------------------------------------------------------------

    def updateLayerList(self, existingOutputGpkg):
        """Refresh status icons when the target GPKG file changes."""
        self._in_gpkg_ids = set()
        self._tree.blockSignals(True)
        try:
            self._refresh_status(self._tree.invisibleRootItem(), existingOutputGpkg)
        finally:
            self._tree.blockSignals(False)

    def getSelectedLayers(self):
        """Return {layerid: layerType} for checked, non-project, non-gpkg layers."""
        result = {}
        self._collect_selected(self._tree.invisibleRootItem(), result)
        return result

    def getUnselectedProjectLayers(self):
        """Return {layerid: QgsMapLayer} for unchecked project/plugin layers."""
        result = {}
        self._collect_unselected_project(self._tree.invisibleRootItem(), result)
        return result

    # ------------------------------------------------------------------
    # Tree construction
    # ------------------------------------------------------------------

    def _build_tree(self, tree_node, parent_item):
        """Recursively mirror the QGIS layer tree into QTreeWidgetItems."""
        all_providers = self.SUPPORTED_PROVIDERS + self.PROJECT_LAYER_PROVIDERS

        for child in tree_node.children():
            if QgsLayerTree.isGroup(child):
                grp_item = QTreeWidgetItem(parent_item)
                grp_item.setText(0, child.name())
                grp_item.setIcon(0, QIcon(QgsLayerTreeModel.iconGroup()))
                grp_item.setFlags(
                    Qt.ItemFlag.ItemIsEnabled
                    | Qt.ItemFlag.ItemIsUserCheckable
                    | Qt.ItemFlag.ItemIsAutoTristate
                )
                grp_item.setCheckState(0, Qt.CheckState.Checked)
                self._build_tree(child, grp_item)
                # Drop groups that ended up empty (all children unsupported)
                if grp_item.childCount() == 0:
                    parent_item.removeChild(grp_item)

            elif QgsLayerTree.isLayer(child):
                layer = child.layer()
                if layer is None:
                    continue
                provider = layer.dataProvider().name() if layer.dataProvider() else ""
                if provider not in all_providers:
                    continue

                size = self._get_layer_size(layer)
                is_large = size is None or size >= self.WARN_SIZE

                item = QTreeWidgetItem(parent_item)
                item.setText(0, layer.name())
                item.setData(0, _LAYER_ID_ROLE, layer.id())
                item.setIcon(0, QgsIconUtils.iconForLayer(layer))
                item.setFlags(
                    Qt.ItemFlag.ItemIsEnabled
                    | Qt.ItemFlag.ItemIsUserCheckable
                )
                item.setCheckState(
                    0,
                    Qt.CheckState.Unchecked if is_large else Qt.CheckState.Checked,
                )
                if is_large:
                    item.setIcon(1, self._icon_warn)
                    item.setToolTip(1, self.tr(
                        "Layer size is unknown or larger than 50 MB – deselected by default"
                    ))

    # ------------------------------------------------------------------
    # Status refresh (called after target GPKG file is chosen)
    # ------------------------------------------------------------------

    def _refresh_status(self, parent_item, existingOutputGpkg):
        for i in range(parent_item.childCount()):
            item = parent_item.child(i)
            layer_id = item.data(0, _LAYER_ID_ROLE)

            if layer_id is None:
                # group node — recurse
                self._refresh_status(item, existingOutputGpkg)
                continue

            layer = QgsProject.instance().mapLayer(layer_id)
            if layer is None:
                continue

            in_gpkg = existingOutputGpkg and (
                layer.source().startswith(existingOutputGpkg)
                or layer.source().startswith("GPKG:" + existingOutputGpkg)
            )
            if in_gpkg:
                self._in_gpkg_ids.add(layer_id)
                item.setCheckState(0, Qt.CheckState.Unchecked)
                item.setFlags(item.flags() & ~Qt.ItemFlag.ItemIsEnabled)
                item.setIcon(1, self._icon_ok)
                item.setToolTip(1, self.tr("Layer is already part of the output GeoPackage"))
            else:
                item.setFlags(item.flags() | Qt.ItemFlag.ItemIsEnabled)
                size = self._get_layer_size(layer)
                is_large = size is None or size >= self.WARN_SIZE
                if is_large:
                    item.setIcon(1, self._icon_warn)
                    item.setToolTip(1, self.tr(
                        "Layer size is unknown or larger than 50 MB – deselected by default"
                    ))
                else:
                    item.setIcon(1, self._icon_none)
                    item.setToolTip(1, "")

    # ------------------------------------------------------------------
    # Tristate propagation (group -> children)
    # ------------------------------------------------------------------

    def _on_item_changed(self, item, column):
        """Push Checked/Unchecked from a group down to its children.

        Qt's ItemIsAutoTristate already handles the upward sync (child -> parent).
        We only need to push downward when the user clicks a group that is
        fully Checked or Unchecked. PartiallyChecked means mixed — leave children alone.
        """
        if column != 0:
            return
        if item.data(0, _LAYER_ID_ROLE) is not None:
            return  # layer node — nothing to propagate downward
        state = item.checkState(0)
        if state == Qt.CheckState.PartiallyChecked:
            return
        self._tree.blockSignals(True)
        try:
            self._set_subtree_check_state(item, state)
        finally:
            self._tree.blockSignals(False)

    def _set_subtree_check_state(self, parent_item, state):
        """Recursively apply *state* to all enabled children of *parent_item*."""
        for i in range(parent_item.childCount()):
            child = parent_item.child(i)
            if child.flags() & Qt.ItemFlag.ItemIsEnabled:
                child.setCheckState(0, state)
            if child.data(0, _LAYER_ID_ROLE) is None:
                self._set_subtree_check_state(child, state)

    # ------------------------------------------------------------------
    # Select / Deselect all
    # ------------------------------------------------------------------

    def _select_all(self):
        self._tree.blockSignals(True)
        try:
            self._set_subtree_check_state(
                self._tree.invisibleRootItem(), Qt.CheckState.Checked
            )
        finally:
            self._tree.blockSignals(False)

    def _deselect_all(self):
        self._tree.blockSignals(True)
        try:
            self._set_subtree_check_state(
                self._tree.invisibleRootItem(), Qt.CheckState.Unchecked
            )
        finally:
            self._tree.blockSignals(False)

    # ------------------------------------------------------------------
    # Result collection
    # ------------------------------------------------------------------

    def _collect_selected(self, parent_item, result):
        for i in range(parent_item.childCount()):
            item = parent_item.child(i)
            layer_id = item.data(0, _LAYER_ID_ROLE)
            if layer_id is None:
                self._collect_selected(item, result)
                continue
            if layer_id in self._in_gpkg_ids:
                continue
            layer = QgsProject.instance().mapLayer(layer_id)
            if layer is None:
                continue
            provider = layer.dataProvider().name() if layer.dataProvider() else ""
            if provider in self.PROJECT_LAYER_PROVIDERS:
                continue
            if item.checkState(0) == Qt.CheckState.Checked:
                result[layer_id] = layer.type()

    def _collect_unselected_project(self, parent_item, result):
        for i in range(parent_item.childCount()):
            item = parent_item.child(i)
            layer_id = item.data(0, _LAYER_ID_ROLE)
            if layer_id is None:
                self._collect_unselected_project(item, result)
                continue
            layer = QgsProject.instance().mapLayer(layer_id)
            if layer is None:
                continue
            provider = layer.dataProvider().name() if layer.dataProvider() else ""
            if provider not in self.PROJECT_LAYER_PROVIDERS:
                continue
            if item.checkState(0) != Qt.CheckState.Checked:
                result[layer_id] = layer

    # ------------------------------------------------------------------
    # File size helper (preserved from original implementation)
    # ------------------------------------------------------------------

    def _get_layer_size(self, layer):
        """Return the file size in bytes for *layer*, or None if not determinable."""
        if layer.dataProvider() and layer.dataProvider().name() in self.PROJECT_LAYER_PROVIDERS:
            return 0

        filename = layer.source()
        pos = filename.find("|")
        if pos >= 0:
            filename = filename[:pos]
        if filename.startswith("file://"):
            qpos = filename.find("?")
            if qpos == -1:
                filename = filename[7:]
            else:
                filename = filename[7:qpos]
        if filename.startswith("/vsi"):
            filename = re.sub(r"/vsi\w+/", "", filename)
            while not os.path.isfile(filename):
                newfilename = os.path.dirname(filename)
                if newfilename == filename:
                    filename = None
                    break
                filename = newfilename
            if not filename:
                return None

        try:
            return os.path.getsize(filename)
        except Exception:
            return None
