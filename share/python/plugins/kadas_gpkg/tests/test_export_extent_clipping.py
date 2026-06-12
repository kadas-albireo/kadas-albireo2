"""Tests for the annotation-layer extent-clipping helpers in kadas_gpkg_export.

collect_annotation_items_outside_extent() decides which annotation items fall
entirely outside the export extent; strip_annotation_items() removes the
corresponding <item> elements from the serialized project XML. Together they
replace the legacy KadasItemLayer flag-and-strip mechanism.
"""

import pytest
from kadas_gpkg.kadas_gpkg_export import (
    KadasGpkgExport,
    collect_annotation_items_outside_extent,
    strip_annotation_items,
)
from lxml import etree as ET
from qgis.core import (
    QgsAnnotationLayer,
    QgsAnnotationMarkerItem,
    QgsCoordinateReferenceSystem,
    QgsPoint,
    QgsProject,
    QgsRectangle,
)

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------


def _make_annotation_layer(name: str, crs: str = "EPSG:2056") -> QgsAnnotationLayer:
    layer = QgsAnnotationLayer(
        name, QgsAnnotationLayer.LayerOptions(QgsProject.instance().transformContext())
    )
    assert layer.isValid()
    layer.setCrs(QgsCoordinateReferenceSystem(crs))
    return layer


def _add_marker(layer: QgsAnnotationLayer, x: float, y: float) -> str:
    return layer.addItem(QgsAnnotationMarkerItem(QgsPoint(x, y)))


@pytest.fixture()
def project():
    project = QgsProject.instance()
    project.clear()
    yield project
    project.clear()


# ---------------------------------------------------------------------------
# collect_annotation_items_outside_extent
# ---------------------------------------------------------------------------


def test_collect_inside_and_outside(project):
    layer = _make_annotation_layer("annotations")
    inside_id = _add_marker(layer, 2600000, 1200000)
    outside_id = _add_marker(layer, 2700000, 1300000)
    project.addMapLayer(layer)

    extent = QgsRectangle(2590000, 1190000, 2610000, 1210000)
    result = collect_annotation_items_outside_extent(
        project, extent, QgsCoordinateReferenceSystem("EPSG:2056")
    )

    assert result == {layer.id(): {outside_id}}
    assert inside_id not in result[layer.id()]


def test_collect_all_inside_yields_empty(project):
    layer = _make_annotation_layer("annotations")
    _add_marker(layer, 2600000, 1200000)
    project.addMapLayer(layer)

    extent = QgsRectangle(2500000, 1100000, 2700000, 1300000)
    result = collect_annotation_items_outside_extent(
        project, extent, QgsCoordinateReferenceSystem("EPSG:2056")
    )

    assert result == {}


def test_collect_transforms_extent_crs(project):
    layer = _make_annotation_layer("annotations", "EPSG:2056")
    inside_id = _add_marker(layer, 2600000, 1200000)  # ~ Bern
    outside_id = _add_marker(layer, 2484000, 1110000)  # ~ Geneva
    project.addMapLayer(layer)

    # Extent around Bern given in WGS84
    extent = QgsRectangle(7.3, 46.8, 7.6, 47.1)
    result = collect_annotation_items_outside_extent(
        project, extent, QgsCoordinateReferenceSystem("EPSG:4326")
    )

    assert result == {layer.id(): {outside_id}}
    assert inside_id not in result[layer.id()]


def test_collect_ignores_non_annotation_layers(project):
    from qgis.core import QgsVectorLayer

    vlayer = QgsVectorLayer("Point?crs=EPSG:2056", "points", "memory")
    project.addMapLayer(vlayer)

    extent = QgsRectangle(0, 0, 1, 1)
    result = collect_annotation_items_outside_extent(
        project, extent, QgsCoordinateReferenceSystem("EPSG:2056")
    )

    assert result == {}


# ---------------------------------------------------------------------------
# strip_annotation_items
# ---------------------------------------------------------------------------


def _project_doc(layer_id: str, item_ids: list) -> "ET._ElementTree":
    items_xml = "".join(f'<item type="marker" id="{i}"/>' for i in item_ids)
    xml = f"""<qgis>
      <projectlayers>
        <maplayer type="annotation">
          <id>{layer_id}</id>
          <items>{items_xml}</items>
        </maplayer>
        <maplayer type="vector">
          <id>vector_layer</id>
        </maplayer>
      </projectlayers>
    </qgis>"""
    return ET.ElementTree(ET.fromstring(xml))


def test_strip_removes_only_matching_items():
    doc = _project_doc("anno_layer", ["keep1", "drop1", "keep2", "drop2"])

    strip_annotation_items(doc, {"anno_layer": {"drop1", "drop2"}})

    remaining = [el.attrib["id"] for el in doc.iterfind("projectlayers/maplayer/items/item")]
    assert remaining == ["keep1", "keep2"]


def test_strip_unknown_layer_is_noop():
    doc = _project_doc("anno_layer", ["a", "b"])

    strip_annotation_items(doc, {"other_layer": {"a"}})

    remaining = [el.attrib["id"] for el in doc.iterfind("projectlayers/maplayer/items/item")]
    assert remaining == ["a", "b"]


def test_strip_empty_mapping_is_noop():
    doc = _project_doc("anno_layer", ["a"])

    strip_annotation_items(doc, {})

    remaining = [el.attrib["id"] for el in doc.iterfind("projectlayers/maplayer/items/item")]
    assert remaining == ["a"]


# ---------------------------------------------------------------------------
# Round trip: real QgsAnnotationLayer serialization → strip
# ---------------------------------------------------------------------------


def test_round_trip_with_real_project_serialization(project, tmp_path):
    layer = _make_annotation_layer("annotations")
    inside_id = _add_marker(layer, 2600000, 1200000)
    outside_id = _add_marker(layer, 2700000, 1300000)
    project.addMapLayer(layer)

    extent = QgsRectangle(2590000, 1190000, 2610000, 1210000)
    to_remove = collect_annotation_items_outside_extent(
        project, extent, QgsCoordinateReferenceSystem("EPSG:2056")
    )

    project_file = str(tmp_path / "project.qgs")
    assert project.write(project_file)

    parser = ET.XMLParser(strip_cdata=False)
    doc = ET.parse(project_file, parser=parser)

    strip_annotation_items(doc, to_remove)

    remaining = [el.attrib["id"] for el in doc.iterfind("projectlayers/maplayer/items/item")]
    assert remaining == [inside_id]
    assert outside_id not in remaining


def test_exporter_hook_sequence_strips_items(project, tmp_path):
    """Replays the hook call order of KadasGpkgExport.run().

    Regression test: the collected item ids must survive until
    __removeFlaggedRedlining runs on the parsed XML (they were once reset
    in between, turning extent clipping into a silent no-op).
    """
    layer = _make_annotation_layer("annotations")
    inside_id = _add_marker(layer, 2600000, 1200000)
    _add_marker(layer, 2700000, 1300000)
    project.addMapLayer(layer)

    exporter = KadasGpkgExport(None)
    extent = QgsRectangle(2590000, 1190000, 2610000, 1210000)

    # Same sequence as run(): collect, write the project, parse, strip
    exporter._KadasGpkgExport__flagRedliningItemsOutsideExtent(
        extent, QgsCoordinateReferenceSystem("EPSG:2056")
    )

    project_file = str(tmp_path / "project.qgs")
    assert project.write(project_file)

    parser = ET.XMLParser(strip_cdata=False)
    doc = ET.parse(project_file, parser=parser)

    exporter._KadasGpkgExport__removeFlaggedRedlining(doc)

    remaining = [el.attrib["id"] for el in doc.iterfind("projectlayers/maplayer/items/item")]
    assert remaining == [inside_id]
