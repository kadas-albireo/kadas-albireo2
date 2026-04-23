import os
import re

from qgis.core import QgsIconUtils, QgsLayerTree, QgsLayerTreeModel, QgsMapLayerType, QgsProject
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
from qgis.PyQt.QtXml import QDomDocument

# Role used to store the layer id on leaf items
_LAYER_ID_ROLE = Qt.ItemDataRole.UserRole + 1

# Map from QGIS project XML layer type strings to QgsMapLayerType enum values
_LAYER_TYPE_MAP = {
    "vector": QgsMapLayerType.VectorLayer,
    "raster": QgsMapLayerType.RasterLayer,
    "mesh": QgsMapLayerType.MeshLayer,
    "vector-tile": QgsMapLayerType.VectorTileLayer,
    "point-cloud": QgsMapLayerType.PointCloudLayer,
}


class KadasGpkgLayersListBase(QWidget):
    """Shared base for the GPKG layer-selection tree widgets.

    Provides the two-column QTreeWidget, tristate group propagation,
    Select All / Deselect All buttons, and the _get_layer_size helper.
    Concrete subclasses build the tree content in their own __init__.
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
        self._icon_ok = QIcon(":/images/themes/default/mIconSuccess.svg")
        self._icon_none = QIcon()

        # layer-ids already present in the target GPKG (export) or already loaded (import)
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

        # itemChanged drives tristate propagation (group -> children)
        self._tree.itemChanged.connect(self._on_item_changed)

        # Buttons
        btn_all = QPushButton(self.tr("Select All"))
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
        """Recursively apply *state* to all enabled children of *parent_item*.

        For group items, we recurse into children FIRST before calling
        setCheckState on the group. Qt's ItemIsAutoTristate propagates changes
        upward via model signals. When blockSignals is active that upward
        propagation is suppressed, so the group's model dataChanged notification
        fires at the moment setCheckState is called on it. If we set the group
        before updating its children, the notification fires while children are
        still in the old (mixed) state and the view renders the group with the
        wrong indicator. Recursing first ensures children are already in the
        target state when the group notification fires.
        """
        for i in range(parent_item.childCount()):
            child = parent_item.child(i)
            if child.data(0, _LAYER_ID_ROLE) is None:
                # Group: descend into children first, then set the group itself.
                self._set_subtree_check_state(child, state)
            if child.flags() & Qt.ItemFlag.ItemIsEnabled:
                child.setCheckState(0, state)

    # ------------------------------------------------------------------
    # Select / Deselect all
    # ------------------------------------------------------------------

    def _select_all(self):
        self._tree.blockSignals(True)
        try:
            self._set_subtree_check_state(self._tree.invisibleRootItem(), Qt.CheckState.Checked)
        finally:
            self._tree.blockSignals(False)

    def _deselect_all(self):
        self._tree.blockSignals(True)
        try:
            self._set_subtree_check_state(self._tree.invisibleRootItem(), Qt.CheckState.Unchecked)
        finally:
            self._tree.blockSignals(False)

    # ------------------------------------------------------------------
    # File size helper
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


class KadasGpkgLayersList(KadasGpkgLayersListBase):
    """Export layer-selection widget — mirrors the live QGIS project layer tree.

    Displays layers grouped and ordered exactly as in the Kadas main window.
    Group checkboxes support tristate (checked / partial / unchecked).
    A second column shows a warning icon for large/unknown-size layers,
    or a success icon for layers already present in the target GeoPackage.

    Public API (unchanged from original):
        updateLayerList(existingOutputGpkg)
        getSelectedLayers()              -> {layerid: layerType}
        getUnselectedProjectLayers()     -> {layerid: QgsMapLayer}
    """

    def __init__(self, parent=None):
        KadasGpkgLayersListBase.__init__(self, parent)

        # Populate from the live project layer tree
        root = QgsProject.instance().layerTreeRoot()
        self._build_tree(root, self._tree.invisibleRootItem())
        self._tree.expandAll()

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
                item.setFlags(Qt.ItemFlag.ItemIsEnabled | Qt.ItemFlag.ItemIsUserCheckable)
                item.setCheckState(
                    0,
                    Qt.CheckState.Unchecked if is_large else Qt.CheckState.Checked,
                )
                if is_large:
                    item.setIcon(1, self._icon_warn)
                    item.setToolTip(
                        1,
                        self.tr(
                            "Layer size is unknown or larger than 50 MB – deselected by default"
                        ),
                    )

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
                    item.setToolTip(
                        1,
                        self.tr(
                            "Layer size is unknown or larger than 50 MB – deselected by default"
                        ),
                    )
                else:
                    item.setIcon(1, self._icon_none)
                    item.setToolTip(1, "")

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


class KadasGpkgImportLayersList(KadasGpkgLayersListBase):
    """Import layer-selection widget — mirrors the layer tree embedded in a GPKG file.

    The tree is initially empty. Call loadFromGpkg() after reading the project XML
    from a GPKG file. Group structure is reconstructed from the <layer-tree-group>
    element in the embedded project XML.

    Layers already loaded in the current QGIS project from the same GPKG file are
    shown with a success icon and disabled. All other layers start unchecked.

    Public API:
        loadFromGpkg(gpkg_filename, xml_bytes)
        getSelectedLayerIds()            -> list[str]
    """

    def __init__(self, parent=None):
        KadasGpkgLayersListBase.__init__(self, parent)

        # path to the GPKG currently shown (used for already-loaded detection)
        self._gpkg_filename = None

    # ------------------------------------------------------------------
    # Public API
    # ------------------------------------------------------------------

    def loadFromGpkg(self, gpkg_filename, xml_bytes):
        """Rebuild the tree from the QGIS project XML embedded in a GPKG file.

        *gpkg_filename* is the path to the .gpkg file.
        *xml_bytes* is the raw project XML string/bytes from the qgis_projects table.
        """
        self._gpkg_filename = gpkg_filename
        self._in_gpkg_ids = set()

        self._tree.blockSignals(True)
        self._tree.invisibleRootItem().takeChildren()
        self._tree.blockSignals(False)

        doc = QDomDocument()
        doc.setContent(xml_bytes)

        # Build a lookup: layer_id -> {name, type, provider}
        maplayer_info = {}
        maplayer_nodes = doc.elementsByTagName("maplayer")
        for i in range(maplayer_nodes.count()):
            el = maplayer_nodes.at(i).toElement()
            id_el = el.firstChildElement("id")
            name_el = el.firstChildElement("layername")
            provider_el = el.firstChildElement("provider")
            if id_el.isNull() or name_el.isNull():
                continue
            maplayer_info[id_el.text()] = {
                "name": name_el.text(),
                "type": el.attribute("type", ""),
                "provider": provider_el.text() if not provider_el.isNull() else "",
            }

        # Reconstruct the layer-tree hierarchy from <layer-tree-group>
        root_group = doc.documentElement().firstChildElement("layer-tree-group")
        self._tree.blockSignals(True)
        try:
            if root_group.isNull():
                # Fallback: flat list in declaration order
                for layer_id, info in maplayer_info.items():
                    self._add_layer_item(self._tree.invisibleRootItem(), layer_id, info)
            else:
                self._build_tree_from_xml(root_group, self._tree.invisibleRootItem(), maplayer_info)
        finally:
            self._tree.blockSignals(False)

        self._tree.expandAll()

    def getSelectedLayerIds(self):
        """Return the IDs of all checked (and not already-loaded) layers."""
        result = []
        self._collect_checked_ids(self._tree.invisibleRootItem(), result)
        return result

    # ------------------------------------------------------------------
    # Tree construction from project XML
    # ------------------------------------------------------------------

    def _build_tree_from_xml(self, group_el, parent_item, maplayer_info):
        """Recursively build QTreeWidgetItems from a <layer-tree-group> element."""
        child = group_el.firstChildElement()
        while not child.isNull():
            tag = child.tagName()
            if tag == "layer-tree-group":
                name = child.attribute("name", "")
                grp_item = QTreeWidgetItem(parent_item)
                grp_item.setText(0, name)
                grp_item.setIcon(0, QIcon(QgsLayerTreeModel.iconGroup()))
                grp_item.setFlags(
                    Qt.ItemFlag.ItemIsEnabled
                    | Qt.ItemFlag.ItemIsUserCheckable
                    | Qt.ItemFlag.ItemIsAutoTristate
                )
                grp_item.setCheckState(0, Qt.CheckState.Unchecked)
                self._build_tree_from_xml(child, grp_item, maplayer_info)
                # Drop groups that ended up with no known layers
                if grp_item.childCount() == 0:
                    parent_item.removeChild(grp_item)
            elif tag == "layer-tree-layer":
                layer_id = child.attribute("id", "")
                info = maplayer_info.get(layer_id)
                if info is not None:
                    self._add_layer_item(parent_item, layer_id, info)
            child = child.nextSiblingElement()

    def _add_layer_item(self, parent_item, layer_id, info):
        """Append a single layer leaf item to *parent_item*."""
        item = QTreeWidgetItem(parent_item)
        item.setText(0, info["name"])
        item.setData(0, _LAYER_ID_ROLE, layer_id)
        item.setIcon(0, self._icon_for_type(info["type"]))

        if self._is_already_loaded(layer_id):
            self._in_gpkg_ids.add(layer_id)
            item.setCheckState(0, Qt.CheckState.Unchecked)
            item.setFlags(Qt.ItemFlag.ItemIsUserCheckable)  # disabled
            item.setIcon(1, self._icon_ok)
            item.setToolTip(1, self.tr("Layer is already loaded from this GeoPackage"))
        else:
            item.setFlags(Qt.ItemFlag.ItemIsEnabled | Qt.ItemFlag.ItemIsUserCheckable)
            item.setCheckState(0, Qt.CheckState.Unchecked)

    def _icon_for_type(self, layer_type_str):
        """Return a QIcon for a layer type string from the project XML."""
        qgs_type = _LAYER_TYPE_MAP.get(layer_type_str.lower())
        if qgs_type is not None:
            try:
                return QgsIconUtils.iconForLayerType(qgs_type)
            except Exception:
                pass
        return QIcon()

    def _is_already_loaded(self, layer_id):
        """Return True if *layer_id* is currently loaded in QGIS from the same GPKG."""
        if not self._gpkg_filename:
            return False
        layer = QgsProject.instance().mapLayer(layer_id)
        if layer is None:
            return False
        src = layer.source()
        return src.startswith(self._gpkg_filename) or src.startswith(
            "GPKG:" + self._gpkg_filename
        )

    # ------------------------------------------------------------------
    # Result collection
    # ------------------------------------------------------------------

    def _collect_checked_ids(self, parent_item, result):
        for i in range(parent_item.childCount()):
            item = parent_item.child(i)
            layer_id = item.data(0, _LAYER_ID_ROLE)
            if layer_id is None:
                self._collect_checked_ids(item, result)
                continue
            if layer_id in self._in_gpkg_ids:
                continue
            if item.checkState(0) == Qt.CheckState.Checked:
                result.append(layer_id)
