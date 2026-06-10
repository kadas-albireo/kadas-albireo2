/***************************************************************************
    testkadasprojectmigration.cpp
    -----------------------------
    copyright            : (C) 2026 by Denis Rouzaud
    email                : denis at opengis dot ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <QDomDocument>
#include <QPair>
#include <QSet>
#include <QStandardPaths>
#include <QStringList>
#include <QTextStream>
#include <QtTest/QTest>

#include <qgis/qgsapplication.h>

#include <kadas/gui/annotationitems/kadasannotationitemcontrollers.h>
#include <kadas/gui/kadasprojectmigration.h>


/**
 * Tests for legacy `.qgs` / `.qgz` project migration.
 *
 * Focus: legacy `KadasMilxLayer` plugin layers are translated into
 * `QgsAnnotationLayer` blocks, with every `KadasMilxItem` payload
 * preserved — including those whose JSON contains non-ASCII characters
 * (e.g. French `militaryName` "Tempête"), which a previous
 * `toLocal8Bit()` call corrupted on Windows (CP1252).
 */
class TestKadasProjectMigration : public QObject
{
    Q_OBJECT

  private slots:
    void initTestCase();

    void migrateLegacyMilxLayer_preservesNonAsciiItems();
    void migrateLegacyKadasItemLayer_translatesPointItem();
    void migrateLegacyKadasItemLayer_translatesTextItem();
    void migrateLegacyKadasItemLayer_translatesLineItem();
    void migrateLegacyKadasItemLayer_translatesPolygonItem();
    void migrateLegacyKadasItemLayer_translatesRectangleItem();
    void migrateLegacyKadasItemLayer_translatesCircleItem();
    void migrateLegacyKadasItemLayer_translatesPictureItem();
    void migrateLegacyKadasItemLayer_translatesSymbolItem();
    void migrateLegacyKadasItemLayer_translatesPinItem();
    void migrateLegacyKadasItemLayer_translatesCircularSectorItem();
    void migrateLegacyKadasItemLayer_translatesGpxRouteItem();
    void migrateLegacyKadasItemLayer_translatesGpxWaypointItem();
    void migrateLegacyKadasItemLayer_leavesV1FormatAlone();
    void migrateLegacyKadasItemLayer_leavesUnknownItemTypeAlone();
    void migrateLegacyPluginGridLayers_translatesBullseyeAndGuideGrid();

  private:
    static QString milxItemCdata( const QString &militaryName, double lon, double lat );
    static QDomElement appendKadasItemLayer( QDomDocument &doc, QDomElement &projectLayersEl, const QString &layerId, const QString &layerName, const QString &authid );
    static void appendV2PointMapItem( QDomDocument &doc, QDomElement &mapLayerEl, double x, double y, const QString &authid, const QString &tooltip = QString() );
};


void TestKadasProjectMigration::initTestCase()
{
  QStandardPaths::setTestModeEnabled( true );
  QgsApplication::init();
  // Registers `KadasMilxAnnotationItem` with QgsAnnotationItemRegistry so
  // the migrated annotation layer can be serialised back to XML.
  KadasAnnotationItemControllers::registerBuiltins();
}

QString TestKadasProjectMigration::milxItemCdata( const QString &militaryName, double lon, double lat )
{
  // Mirror the JSON shape produced by the legacy KadasMilxLayer
  // serializer for a single-point symbol. `militaryName` is the field
  // that historically carried non-ASCII text and triggered the
  // `toLocal8Bit()` corruption bug.
  return QStringLiteral(
           "{\"props\":{\"authId\":\"EPSG:4326\",\"editor\":\"KadasMilxEditor\","
           "\"hasVariablePoints\":false,\"militaryName\":\"%1\",\"minNPoints\":1,"
           "\"mssString\":\"<Symbol ID=\\\"EFNP-----------\\\"><Attribute "
           "ID=\\\"XE\\\">9SCHOII--------</Attribute></Symbol>\","
           "\"objectName\":\"\",\"symbolScale\":1,\"symbolType\":\"Other\","
           "\"tooltip\":\"\",\"zIndex\":0},\"state\":{\"attributePoints\":[],"
           "\"attributes\":[],\"controlPoints\":[],\"margin\":[13,13,12,12],"
           "\"points\":[[%2,%3]],\"pressedPoints\":1,\"status\":2,"
           "\"userOffset\":[0,0]}}"
  )
    .arg( militaryName )
    .arg( lon, 0, 'f', 6 )
    .arg( lat, 0, 'f', 6 );
}

void TestKadasProjectMigration::migrateLegacyMilxLayer_preservesNonAsciiItems()
{
  // Build a minimal `<qgis>` project document with one legacy MilX
  // plugin layer carrying two items:
  //   * one with an ASCII `militaryName` ("Bezier")
  //   * one with a UTF-8 `militaryName` ("Tempête") — the regression
  //     case: `toLocal8Bit()` would CP1252-mangle the `ê` byte sequence
  //     on Windows, producing invalid JSON, an empty `mssString`, and
  //     the item being silently dropped.
  QDomDocument doc;
  QDomElement root = doc.createElement( QStringLiteral( "qgis" ) );
  doc.appendChild( root );
  QDomElement projectLayersEl = doc.createElement( QStringLiteral( "projectlayers" ) );
  root.appendChild( projectLayersEl );

  QDomElement mapLayerEl = doc.createElement( QStringLiteral( "maplayer" ) );
  mapLayerEl.setAttribute( QStringLiteral( "type" ), QStringLiteral( "plugin" ) );
  mapLayerEl.setAttribute( QStringLiteral( "name" ), QStringLiteral( "KadasMilxLayer" ) );
  projectLayersEl.appendChild( mapLayerEl );

  QDomElement idEl = doc.createElement( QStringLiteral( "id" ) );
  idEl.appendChild( doc.createTextNode( QStringLiteral( "MilX_test_layer_id" ) ) );
  mapLayerEl.appendChild( idEl );
  QDomElement nameEl = doc.createElement( QStringLiteral( "layername" ) );
  nameEl.appendChild( doc.createTextNode( QStringLiteral( "MilX" ) ) );
  mapLayerEl.appendChild( nameEl );

  const auto appendMapItem = [&]( const QString &cdata ) {
    QDomElement itemEl = doc.createElement( QStringLiteral( "MapItem" ) );
    itemEl.setAttribute( QStringLiteral( "editor" ), QStringLiteral( "KadasMilxEditor" ) );
    itemEl.setAttribute( QStringLiteral( "name" ), QStringLiteral( "KadasMilxItem" ) );
    itemEl.setAttribute( QStringLiteral( "crs" ), QStringLiteral( "EPSG:4326" ) );
    itemEl.appendChild( doc.createCDATASection( cdata ) );
    mapLayerEl.appendChild( itemEl );
  };
  appendMapItem( milxItemCdata( QStringLiteral( "Bezier" ), 8.27, 46.22 ) );
  appendMapItem( milxItemCdata( QStringLiteral( "Tempête" ), 8.75, 46.02 ) );

  QStringList filesToAttach;
  const bool ok = KadasProjectMigration::migrateProjectXml( QString(), doc, filesToAttach );
  QVERIFY( ok );

  // After migration the legacy plugin layer must be replaced by a
  // QgsAnnotationLayer block.
  const QDomElement migratedRoot = doc.documentElement();
  const QDomElement migratedProjectLayers = migratedRoot.firstChildElement( QStringLiteral( "projectlayers" ) );
  QVERIFY( !migratedProjectLayers.isNull() );
  const QDomElement migratedLayer = migratedProjectLayers.firstChildElement( QStringLiteral( "maplayer" ) );
  QVERIFY( !migratedLayer.isNull() );
  QCOMPARE( migratedLayer.attribute( QStringLiteral( "type" ) ), QStringLiteral( "annotation" ) );
  // Original layer id must be preserved so layer-tree-layer references
  // continue to resolve.
  QCOMPARE( migratedLayer.firstChildElement( QStringLiteral( "id" ) ).text(), QStringLiteral( "MilX_test_layer_id" ) );

  // Both items must round-trip through migration. Pre-fix, on Windows
  // (CP1252) the `Tempête` item was silently dropped because the
  // CDATA payload was decoded with `QString::toLocal8Bit()` before
  // being handed to `QJsonDocument::fromJson`, mangling the UTF-8
  // `ê` and producing a null JSON object. On platforms whose
  // `toLocal8Bit()` is UTF-8 (Linux, macOS) this test acts as a
  // positive guard — keeping the migration honest if anyone ever
  // re-introduces a locale-dependent decode.
  const QDomElement itemsEl = migratedLayer.firstChildElement( QStringLiteral( "items" ) );
  QVERIFY( !itemsEl.isNull() );
  const QDomNodeList items = itemsEl.elementsByTagName( QStringLiteral( "item" ) );
  QCOMPARE( items.size(), 2 );
}

QDomElement TestKadasProjectMigration::appendKadasItemLayer( QDomDocument &doc, QDomElement &projectLayersEl, const QString &layerId, const QString &layerName, const QString &authid )
{
  QDomElement mapLayerEl = doc.createElement( QStringLiteral( "maplayer" ) );
  mapLayerEl.setAttribute( QStringLiteral( "type" ), QStringLiteral( "plugin" ) );
  mapLayerEl.setAttribute( QStringLiteral( "name" ), QStringLiteral( "KadasItemLayer" ) );
  mapLayerEl.setAttribute( QStringLiteral( "title" ), layerName );
  projectLayersEl.appendChild( mapLayerEl );

  QDomElement srsEl = doc.createElement( QStringLiteral( "srs" ) );
  QDomElement srsInner = doc.createElement( QStringLiteral( "spatialrefsys" ) );
  QDomElement authidEl = doc.createElement( QStringLiteral( "authid" ) );
  authidEl.appendChild( doc.createTextNode( authid ) );
  srsInner.appendChild( authidEl );
  srsEl.appendChild( srsInner );
  mapLayerEl.appendChild( srsEl );

  QDomElement idEl = doc.createElement( QStringLiteral( "id" ) );
  idEl.appendChild( doc.createTextNode( layerId ) );
  mapLayerEl.appendChild( idEl );

  QDomElement nameEl = doc.createElement( QStringLiteral( "layername" ) );
  nameEl.appendChild( doc.createTextNode( layerName ) );
  mapLayerEl.appendChild( nameEl );

  return mapLayerEl;
}

void TestKadasProjectMigration::appendV2PointMapItem( QDomDocument &doc, QDomElement &mapLayerEl, double x, double y, const QString &authid, const QString &tooltip )
{
  QDomElement itemEl = doc.createElement( QStringLiteral( "MapItem" ) );
  itemEl.setAttribute( QStringLiteral( "name" ), QStringLiteral( "KadasPointItem" ) );
  itemEl.setAttribute( QStringLiteral( "crs" ), authid );
  itemEl.setAttribute( QStringLiteral( "editor" ), QStringLiteral( "KadasRedliningEditor" ) );
  itemEl.setAttribute( QStringLiteral( "draw_status" ), QStringLiteral( "Finished" ) );
  itemEl.setAttribute( QStringLiteral( "format_version" ), QStringLiteral( "2" ) );
  itemEl.setAttribute( QStringLiteral( "z_index" ), QStringLiteral( "42" ) );
  itemEl.setAttribute( QStringLiteral( "symbol_scale" ), QStringLiteral( "1" ) );
  if ( !tooltip.isEmpty() )
    itemEl.setAttribute( QStringLiteral( "tooltip" ), tooltip );
  itemEl.setAttribute( QStringLiteral( "shape" ), QStringLiteral( "Circle" ) );
  itemEl.setAttribute( QStringLiteral( "size" ), QStringLiteral( "6" ) );
  itemEl.setAttribute( QStringLiteral( "stroke_color" ), QStringLiteral( "#ff0000" ) );
  itemEl.setAttribute( QStringLiteral( "stroke_width" ), QStringLiteral( "2" ) );
  itemEl.setAttribute( QStringLiteral( "fill_color" ), QStringLiteral( "#ffff00" ) );
  itemEl.setAttribute( QStringLiteral( "geometry" ), QStringLiteral( "POINT(%1 %2)" ).arg( x, 0, 'f', 6 ).arg( y, 0, 'f', 6 ) );
  mapLayerEl.appendChild( itemEl );
}

void TestKadasProjectMigration::migrateLegacyKadasItemLayer_translatesPointItem()
{
  // Single legacy `KadasItemLayer` carrying two v2 `KadasPointItem`
  // entries. After migration the layer must become a stock annotation
  // layer with two `<item>` children, the original layer id and name
  // preserved, and the tooltip stored as a custom property.
  QDomDocument doc;
  QDomElement root = doc.createElement( QStringLiteral( "qgis" ) );
  doc.appendChild( root );
  QDomElement projectLayersEl = doc.createElement( QStringLiteral( "projectlayers" ) );
  root.appendChild( projectLayersEl );

  QDomElement mapLayerEl = appendKadasItemLayer( doc, projectLayersEl, QStringLiteral( "redlining_id" ), QStringLiteral( "Redlining" ), QStringLiteral( "EPSG:3857" ) );
  appendV2PointMapItem( doc, mapLayerEl, 920000.0, 5800000.0, QStringLiteral( "EPSG:3857" ), QStringLiteral( "first" ) );
  appendV2PointMapItem( doc, mapLayerEl, 925000.0, 5810000.0, QStringLiteral( "EPSG:3857" ) );

  QStringList filesToAttach;
  QVERIFY( KadasProjectMigration::migrateProjectXml( QString(), doc, filesToAttach ) );

  const QDomElement migratedLayer = doc.documentElement().firstChildElement( QStringLiteral( "projectlayers" ) ).firstChildElement( QStringLiteral( "maplayer" ) );
  QVERIFY( !migratedLayer.isNull() );
  QCOMPARE( migratedLayer.attribute( QStringLiteral( "type" ) ), QStringLiteral( "annotation" ) );
  QCOMPARE( migratedLayer.firstChildElement( QStringLiteral( "id" ) ).text(), QStringLiteral( "redlining_id" ) );
  QCOMPARE( migratedLayer.firstChildElement( QStringLiteral( "layername" ) ).text(), QStringLiteral( "Redlining" ) );

  const QDomNodeList items = migratedLayer.firstChildElement( QStringLiteral( "items" ) ).elementsByTagName( QStringLiteral( "item" ) );
  QCOMPARE( items.size(), 2 );
}

void TestKadasProjectMigration::migrateLegacyKadasItemLayer_translatesTextItem()
{
  // Same shape as the Point case but the MapItem is a KadasTextItem;
  // verifies the dispatcher routes by `name` and the
  // QgsAnnotationPointTextItem round-trips through writeLayerXml.
  QDomDocument doc;
  QDomElement root = doc.createElement( QStringLiteral( "qgis" ) );
  doc.appendChild( root );
  QDomElement projectLayersEl = doc.createElement( QStringLiteral( "projectlayers" ) );
  root.appendChild( projectLayersEl );

  QDomElement mapLayerEl = appendKadasItemLayer( doc, projectLayersEl, QStringLiteral( "text_id" ), QStringLiteral( "Labels" ), QStringLiteral( "EPSG:3857" ) );

  QDomElement itemEl = doc.createElement( QStringLiteral( "MapItem" ) );
  itemEl.setAttribute( QStringLiteral( "name" ), QStringLiteral( "KadasTextItem" ) );
  itemEl.setAttribute( QStringLiteral( "crs" ), QStringLiteral( "EPSG:3857" ) );
  itemEl.setAttribute( QStringLiteral( "format_version" ), QStringLiteral( "2" ) );
  itemEl.setAttribute( QStringLiteral( "text" ), QStringLiteral( "Hello" ) );
  itemEl.setAttribute( QStringLiteral( "color" ), QStringLiteral( "#000000" ) );
  itemEl.setAttribute( QStringLiteral( "outline_color" ), QStringLiteral( "#ffffff" ) );
  itemEl.setAttribute( QStringLiteral( "font" ), QStringLiteral( "Helvetica,12,-1,5,50,0,0,0,0,0" ) );
  itemEl.setAttribute( QStringLiteral( "angle" ), QStringLiteral( "30" ) );
  itemEl.setAttribute( QStringLiteral( "geometry" ), QStringLiteral( "POINT(920000.0 5800000.0)" ) );
  mapLayerEl.appendChild( itemEl );

  QStringList filesToAttach;
  QVERIFY( KadasProjectMigration::migrateProjectXml( QString(), doc, filesToAttach ) );

  const QDomElement migratedLayer = doc.documentElement().firstChildElement( QStringLiteral( "projectlayers" ) ).firstChildElement( QStringLiteral( "maplayer" ) );
  QCOMPARE( migratedLayer.attribute( QStringLiteral( "type" ) ), QStringLiteral( "annotation" ) );
  const QDomNodeList items = migratedLayer.firstChildElement( QStringLiteral( "items" ) ).elementsByTagName( QStringLiteral( "item" ) );
  QCOMPARE( items.size(), 1 );
  QCOMPARE( items.at( 0 ).toElement().attribute( QStringLiteral( "type" ) ), QStringLiteral( "pointtext" ) );
}

void TestKadasProjectMigration::migrateLegacyKadasItemLayer_translatesLineItem()
{
  // KadasLineItem fans a MultiLineString into one QgsAnnotationLineItem
  // per part; here a 2-part input yields 2 annotation items.
  QDomDocument doc;
  QDomElement root = doc.createElement( QStringLiteral( "qgis" ) );
  doc.appendChild( root );
  QDomElement projectLayersEl = doc.createElement( QStringLiteral( "projectlayers" ) );
  root.appendChild( projectLayersEl );

  QDomElement mapLayerEl = appendKadasItemLayer( doc, projectLayersEl, QStringLiteral( "line_id" ), QStringLiteral( "Lines" ), QStringLiteral( "EPSG:3857" ) );

  QDomElement itemEl = doc.createElement( QStringLiteral( "MapItem" ) );
  itemEl.setAttribute( QStringLiteral( "name" ), QStringLiteral( "KadasLineItem" ) );
  itemEl.setAttribute( QStringLiteral( "crs" ), QStringLiteral( "EPSG:3857" ) );
  itemEl.setAttribute( QStringLiteral( "format_version" ), QStringLiteral( "2" ) );
  itemEl.setAttribute( QStringLiteral( "outline_color" ), QStringLiteral( "#ff0000" ) );
  itemEl.setAttribute( QStringLiteral( "outline_width" ), QStringLiteral( "2" ) );
  itemEl.setAttribute( QStringLiteral( "outline_style" ), QString::number( static_cast<int>( Qt::SolidLine ) ) );
  itemEl.setAttribute( QStringLiteral( "fill_color" ), QStringLiteral( "#00000000" ) );
  itemEl.setAttribute( QStringLiteral( "fill_style" ), QString::number( static_cast<int>( Qt::SolidPattern ) ) );
  itemEl.setAttribute( QStringLiteral( "geometry" ), QStringLiteral( "MultiLineString ((0 0, 1 1, 2 0), (10 10, 11 11))" ) );
  mapLayerEl.appendChild( itemEl );

  QStringList filesToAttach;
  QVERIFY( KadasProjectMigration::migrateProjectXml( QString(), doc, filesToAttach ) );

  const QDomElement migratedLayer = doc.documentElement().firstChildElement( QStringLiteral( "projectlayers" ) ).firstChildElement( QStringLiteral( "maplayer" ) );
  QCOMPARE( migratedLayer.attribute( QStringLiteral( "type" ) ), QStringLiteral( "annotation" ) );
  const QDomNodeList items = migratedLayer.firstChildElement( QStringLiteral( "items" ) ).elementsByTagName( QStringLiteral( "item" ) );
  QCOMPARE( items.size(), 2 );
  QCOMPARE( items.at( 0 ).toElement().attribute( QStringLiteral( "type" ) ), QStringLiteral( "linestring" ) );
  QCOMPARE( items.at( 1 ).toElement().attribute( QStringLiteral( "type" ) ), QStringLiteral( "linestring" ) );
}

void TestKadasProjectMigration::migrateLegacyKadasItemLayer_translatesPolygonItem()
{
  // KadasPolygonItem fans a MultiPolygon into one QgsAnnotationPolygonItem
  // per part.
  QDomDocument doc;
  QDomElement root = doc.createElement( QStringLiteral( "qgis" ) );
  doc.appendChild( root );
  QDomElement projectLayersEl = doc.createElement( QStringLiteral( "projectlayers" ) );
  root.appendChild( projectLayersEl );

  QDomElement mapLayerEl = appendKadasItemLayer( doc, projectLayersEl, QStringLiteral( "poly_id" ), QStringLiteral( "Polygons" ), QStringLiteral( "EPSG:3857" ) );

  QDomElement itemEl = doc.createElement( QStringLiteral( "MapItem" ) );
  itemEl.setAttribute( QStringLiteral( "name" ), QStringLiteral( "KadasPolygonItem" ) );
  itemEl.setAttribute( QStringLiteral( "crs" ), QStringLiteral( "EPSG:3857" ) );
  itemEl.setAttribute( QStringLiteral( "format_version" ), QStringLiteral( "2" ) );
  itemEl.setAttribute( QStringLiteral( "outline_color" ), QStringLiteral( "#000000" ) );
  itemEl.setAttribute( QStringLiteral( "outline_width" ), QStringLiteral( "1" ) );
  itemEl.setAttribute( QStringLiteral( "outline_style" ), QString::number( static_cast<int>( Qt::SolidLine ) ) );
  itemEl.setAttribute( QStringLiteral( "fill_color" ), QStringLiteral( "#80ff00ff" ) );
  itemEl.setAttribute( QStringLiteral( "fill_style" ), QString::number( static_cast<int>( Qt::SolidPattern ) ) );
  itemEl.setAttribute( QStringLiteral( "geometry" ), QStringLiteral( "MultiPolygon (((0 0, 10 0, 10 10, 0 10, 0 0)), ((20 20, 30 20, 30 30, 20 30, 20 20)))" ) );
  mapLayerEl.appendChild( itemEl );

  QStringList filesToAttach;
  QVERIFY( KadasProjectMigration::migrateProjectXml( QString(), doc, filesToAttach ) );

  const QDomElement migratedLayer = doc.documentElement().firstChildElement( QStringLiteral( "projectlayers" ) ).firstChildElement( QStringLiteral( "maplayer" ) );
  QCOMPARE( migratedLayer.attribute( QStringLiteral( "type" ) ), QStringLiteral( "annotation" ) );
  const QDomNodeList items = migratedLayer.firstChildElement( QStringLiteral( "items" ) ).elementsByTagName( QStringLiteral( "item" ) );
  QCOMPARE( items.size(), 2 );
  QCOMPARE( items.at( 0 ).toElement().attribute( QStringLiteral( "type" ) ), QStringLiteral( "polygon" ) );
  QCOMPARE( items.at( 1 ).toElement().attribute( QStringLiteral( "type" ) ), QStringLiteral( "polygon" ) );
}

void TestKadasProjectMigration::migrateLegacyKadasItemLayer_translatesRectangleItem()
{
  // KadasRectangleItem stores N (p1, p2) diagonal pairs; each pair fans
  // into one KadasRectangleAnnotationItem.
  QDomDocument doc;
  QDomElement root = doc.createElement( QStringLiteral( "qgis" ) );
  doc.appendChild( root );
  QDomElement projectLayersEl = doc.createElement( QStringLiteral( "projectlayers" ) );
  root.appendChild( projectLayersEl );

  QDomElement mapLayerEl = appendKadasItemLayer( doc, projectLayersEl, QStringLiteral( "rect_id" ), QStringLiteral( "Rects" ), QStringLiteral( "EPSG:3857" ) );

  QDomElement itemEl = doc.createElement( QStringLiteral( "MapItem" ) );
  itemEl.setAttribute( QStringLiteral( "name" ), QStringLiteral( "KadasRectangleItem" ) );
  itemEl.setAttribute( QStringLiteral( "crs" ), QStringLiteral( "EPSG:3857" ) );
  itemEl.setAttribute( QStringLiteral( "format_version" ), QStringLiteral( "2" ) );
  itemEl.setAttribute( QStringLiteral( "outline_color" ), QStringLiteral( "#000000" ) );
  itemEl.setAttribute( QStringLiteral( "outline_width" ), QStringLiteral( "1" ) );
  itemEl.setAttribute( QStringLiteral( "outline_style" ), QString::number( static_cast<int>( Qt::SolidLine ) ) );
  itemEl.setAttribute( QStringLiteral( "fill_color" ), QStringLiteral( "#80ffff00" ) );
  itemEl.setAttribute( QStringLiteral( "fill_style" ), QString::number( static_cast<int>( Qt::SolidPattern ) ) );
  // Two rectangles: (0,0)-(10,10) and (20,20)-(30,40).
  itemEl.setAttribute( QStringLiteral( "p1" ), QStringLiteral( "0,0;20,20" ) );
  itemEl.setAttribute( QStringLiteral( "p2" ), QStringLiteral( "10,10;30,40" ) );
  mapLayerEl.appendChild( itemEl );

  QStringList filesToAttach;
  QVERIFY( KadasProjectMigration::migrateProjectXml( QString(), doc, filesToAttach ) );

  const QDomElement migratedLayer = doc.documentElement().firstChildElement( QStringLiteral( "projectlayers" ) ).firstChildElement( QStringLiteral( "maplayer" ) );
  QCOMPARE( migratedLayer.attribute( QStringLiteral( "type" ) ), QStringLiteral( "annotation" ) );
  const QDomNodeList items = migratedLayer.firstChildElement( QStringLiteral( "items" ) ).elementsByTagName( QStringLiteral( "item" ) );
  QCOMPARE( items.size(), 2 );
  QCOMPARE( items.at( 0 ).toElement().attribute( QStringLiteral( "type" ) ), QStringLiteral( "kadas:rectangle" ) );
  QCOMPARE( items.at( 1 ).toElement().attribute( QStringLiteral( "type" ) ), QStringLiteral( "kadas:rectangle" ) );
  // QgsAnnotationLayer stores items in a hash; on-disk order is not
  // stable across runs. Verify the set of (cx, cy, w, h) tuples.
  QSet<QString> rects;
  for ( int i = 0; i < items.size(); ++i )
  {
    const QDomElement el = items.at( i ).toElement();
    rects.insert( QStringLiteral( "%1,%2,%3,%4" )
                    .arg( el.attribute( QStringLiteral( "cx" ) ).toDouble() )
                    .arg( el.attribute( QStringLiteral( "cy" ) ).toDouble() )
                    .arg( el.attribute( QStringLiteral( "w" ) ).toDouble() )
                    .arg( el.attribute( QStringLiteral( "h" ) ).toDouble() ) );
  }
  QVERIFY( rects.contains( QStringLiteral( "5,5,10,10" ) ) );
  QVERIFY( rects.contains( QStringLiteral( "25,30,10,20" ) ) );
}

void TestKadasProjectMigration::migrateLegacyKadasItemLayer_translatesCircleItem()
{
  // KadasCircleItem stores N (center, ringPoint) pairs; each pair fans
  // into one KadasCircleAnnotationItem (type=kadas:circle).
  QDomDocument doc;
  QDomElement root = doc.createElement( QStringLiteral( "qgis" ) );
  doc.appendChild( root );
  QDomElement projectLayersEl = doc.createElement( QStringLiteral( "projectlayers" ) );
  root.appendChild( projectLayersEl );

  QDomElement mapLayerEl = appendKadasItemLayer( doc, projectLayersEl, QStringLiteral( "circle_id" ), QStringLiteral( "Circles" ), QStringLiteral( "EPSG:3857" ) );

  QDomElement itemEl = doc.createElement( QStringLiteral( "MapItem" ) );
  itemEl.setAttribute( QStringLiteral( "name" ), QStringLiteral( "KadasCircleItem" ) );
  itemEl.setAttribute( QStringLiteral( "crs" ), QStringLiteral( "EPSG:3857" ) );
  itemEl.setAttribute( QStringLiteral( "format_version" ), QStringLiteral( "2" ) );
  itemEl.setAttribute( QStringLiteral( "outline_color" ), QStringLiteral( "#000000" ) );
  itemEl.setAttribute( QStringLiteral( "outline_width" ), QStringLiteral( "1" ) );
  itemEl.setAttribute( QStringLiteral( "outline_style" ), QString::number( static_cast<int>( Qt::SolidLine ) ) );
  itemEl.setAttribute( QStringLiteral( "fill_color" ), QStringLiteral( "#80ff8800" ) );
  itemEl.setAttribute( QStringLiteral( "fill_style" ), QString::number( static_cast<int>( Qt::SolidPattern ) ) );
  itemEl.setAttribute( QStringLiteral( "centers" ), QStringLiteral( "0,0;100,100" ) );
  itemEl.setAttribute( QStringLiteral( "ringpos" ), QStringLiteral( "10,0;105,100" ) );
  mapLayerEl.appendChild( itemEl );

  QStringList filesToAttach;
  QVERIFY( KadasProjectMigration::migrateProjectXml( QString(), doc, filesToAttach ) );

  const QDomElement migratedLayer = doc.documentElement().firstChildElement( QStringLiteral( "projectlayers" ) ).firstChildElement( QStringLiteral( "maplayer" ) );
  QCOMPARE( migratedLayer.attribute( QStringLiteral( "type" ) ), QStringLiteral( "annotation" ) );
  const QDomNodeList items = migratedLayer.firstChildElement( QStringLiteral( "items" ) ).elementsByTagName( QStringLiteral( "item" ) );
  QCOMPARE( items.size(), 2 );
  QCOMPARE( items.at( 0 ).toElement().attribute( QStringLiteral( "type" ) ), QStringLiteral( "kadas:circle" ) );
  QCOMPARE( items.at( 1 ).toElement().attribute( QStringLiteral( "type" ) ), QStringLiteral( "kadas:circle" ) );
  // QgsAnnotationLayer stores items in a hash; the on-disk order is not
  // stable across runs. Verify the set of centers instead.
  QSet<QPair<double, double>> centers;
  for ( int i = 0; i < items.size(); ++i )
  {
    const QDomElement el = items.at( i ).toElement();
    centers.insert( qMakePair( el.attribute( QStringLiteral( "cx" ) ).toDouble(), el.attribute( QStringLiteral( "cy" ) ).toDouble() ) );
  }
  QVERIFY( centers.contains( qMakePair( 0.0, 0.0 ) ) );
  QVERIFY( centers.contains( qMakePair( 100.0, 100.0 ) ) );
}

void TestKadasProjectMigration::migrateLegacyKadasItemLayer_translatesPictureItem()
{
  // KadasPictureItem (v2 format) carries one picture and is translated
  // to a single QgsAnnotationPictureItem (type=picture).
  QDomDocument doc;
  QDomElement root = doc.createElement( QStringLiteral( "qgis" ) );
  doc.appendChild( root );
  QDomElement projectLayersEl = doc.createElement( QStringLiteral( "projectlayers" ) );
  root.appendChild( projectLayersEl );

  QDomElement mapLayerEl = appendKadasItemLayer( doc, projectLayersEl, QStringLiteral( "pic_id" ), QStringLiteral( "Pictures" ), QStringLiteral( "EPSG:3857" ) );

  QDomElement itemEl = doc.createElement( QStringLiteral( "MapItem" ) );
  itemEl.setAttribute( QStringLiteral( "name" ), QStringLiteral( "KadasPictureItem" ) );
  itemEl.setAttribute( QStringLiteral( "crs" ), QStringLiteral( "EPSG:3857" ) );
  itemEl.setAttribute( QStringLiteral( "format_version" ), QStringLiteral( "2" ) );
  itemEl.setAttribute( QStringLiteral( "pos_x" ), QStringLiteral( "100" ) );
  itemEl.setAttribute( QStringLiteral( "pos_y" ), QStringLiteral( "200" ) );
  itemEl.setAttribute( QStringLiteral( "offset_x" ), QStringLiteral( "10" ) );
  itemEl.setAttribute( QStringLiteral( "offset_y" ), QStringLiteral( "20" ) );
  itemEl.setAttribute( QStringLiteral( "size_w" ), QStringLiteral( "300" ) );
  itemEl.setAttribute( QStringLiteral( "size_h" ), QStringLiteral( "250" ) );
  itemEl.setAttribute( QStringLiteral( "frame" ), QStringLiteral( "1" ) );
  itemEl.setAttribute( QStringLiteral( "file_path" ), QStringLiteral( "/tmp/nonexistent.png" ) );
  mapLayerEl.appendChild( itemEl );

  QStringList filesToAttach;
  QVERIFY( KadasProjectMigration::migrateProjectXml( QString(), doc, filesToAttach ) );

  const QDomElement migratedLayer = doc.documentElement().firstChildElement( QStringLiteral( "projectlayers" ) ).firstChildElement( QStringLiteral( "maplayer" ) );
  QCOMPARE( migratedLayer.attribute( QStringLiteral( "type" ) ), QStringLiteral( "annotation" ) );
  const QDomNodeList items = migratedLayer.firstChildElement( QStringLiteral( "items" ) ).elementsByTagName( QStringLiteral( "item" ) );
  QCOMPARE( items.size(), 1 );
  QCOMPARE( items.at( 0 ).toElement().attribute( QStringLiteral( "type" ) ), QStringLiteral( "picture" ) );
}

void TestKadasProjectMigration::migrateLegacyKadasItemLayer_translatesSymbolItem()
{
  // KadasSymbolItem (v2 format) carries one symbol and translates to a
  // single QgsAnnotationPictureItem (type=picture).
  QDomDocument doc;
  QDomElement root = doc.createElement( QStringLiteral( "qgis" ) );
  doc.appendChild( root );
  QDomElement projectLayersEl = doc.createElement( QStringLiteral( "projectlayers" ) );
  root.appendChild( projectLayersEl );

  QDomElement mapLayerEl = appendKadasItemLayer( doc, projectLayersEl, QStringLiteral( "sym_id" ), QStringLiteral( "Symbols" ), QStringLiteral( "EPSG:3857" ) );

  QDomElement itemEl = doc.createElement( QStringLiteral( "MapItem" ) );
  itemEl.setAttribute( QStringLiteral( "name" ), QStringLiteral( "KadasSymbolItem" ) );
  itemEl.setAttribute( QStringLiteral( "crs" ), QStringLiteral( "EPSG:3857" ) );
  itemEl.setAttribute( QStringLiteral( "format_version" ), QStringLiteral( "2" ) );
  itemEl.setAttribute( QStringLiteral( "pos_x" ), QStringLiteral( "500" ) );
  itemEl.setAttribute( QStringLiteral( "pos_y" ), QStringLiteral( "600" ) );
  itemEl.setAttribute( QStringLiteral( "anchor_x" ), QStringLiteral( "0.5" ) );
  itemEl.setAttribute( QStringLiteral( "anchor_y" ), QStringLiteral( "0.5" ) );
  itemEl.setAttribute( QStringLiteral( "size_w" ), QStringLiteral( "32" ) );
  itemEl.setAttribute( QStringLiteral( "size_h" ), QStringLiteral( "32" ) );
  itemEl.setAttribute( QStringLiteral( "file_path" ), QStringLiteral( ":/kadas/icons/pin_red" ) );
  mapLayerEl.appendChild( itemEl );

  QStringList filesToAttach;
  QVERIFY( KadasProjectMigration::migrateProjectXml( QString(), doc, filesToAttach ) );

  const QDomElement migratedLayer = doc.documentElement().firstChildElement( QStringLiteral( "projectlayers" ) ).firstChildElement( QStringLiteral( "maplayer" ) );
  QCOMPARE( migratedLayer.attribute( QStringLiteral( "type" ) ), QStringLiteral( "annotation" ) );
  const QDomNodeList items = migratedLayer.firstChildElement( QStringLiteral( "items" ) ).elementsByTagName( QStringLiteral( "item" ) );
  QCOMPARE( items.size(), 1 );
  QCOMPARE( items.at( 0 ).toElement().attribute( QStringLiteral( "type" ) ), QStringLiteral( "picture" ) );
}

void TestKadasProjectMigration::migrateLegacyKadasItemLayer_translatesPinItem()
{
  // KadasPinItem (v2 format) translates to KadasPinAnnotationItem
  // (type=kadas:pin), carrying name/remarks.
  QDomDocument doc;
  QDomElement root = doc.createElement( QStringLiteral( "qgis" ) );
  doc.appendChild( root );
  QDomElement projectLayersEl = doc.createElement( QStringLiteral( "projectlayers" ) );
  root.appendChild( projectLayersEl );

  QDomElement mapLayerEl = appendKadasItemLayer( doc, projectLayersEl, QStringLiteral( "pin_id" ), QStringLiteral( "Pins" ), QStringLiteral( "EPSG:3857" ) );

  QDomElement itemEl = doc.createElement( QStringLiteral( "MapItem" ) );
  itemEl.setAttribute( QStringLiteral( "name" ), QStringLiteral( "KadasPinItem" ) );
  itemEl.setAttribute( QStringLiteral( "crs" ), QStringLiteral( "EPSG:3857" ) );
  itemEl.setAttribute( QStringLiteral( "format_version" ), QStringLiteral( "2" ) );
  itemEl.setAttribute( QStringLiteral( "pos_x" ), QStringLiteral( "1000" ) );
  itemEl.setAttribute( QStringLiteral( "pos_y" ), QStringLiteral( "2000" ) );
  // NOTE: KadasSymbolItem::writeXmlPrivate clobbers the class-name `name`
  // attribute with the display name (legacy bug). The XML rewriter
  // dispatches on `name`, so we leave the display name empty here and
  // only set `remarks`. Real-world legacy saves with non-empty pin names
  // are unrecoverable by either dispatcher.
  itemEl.setAttribute( QStringLiteral( "remarks" ), QStringLiteral( "test pin" ) );
  mapLayerEl.appendChild( itemEl );

  QStringList filesToAttach;
  QVERIFY( KadasProjectMigration::migrateProjectXml( QString(), doc, filesToAttach ) );

  const QDomElement migratedLayer = doc.documentElement().firstChildElement( QStringLiteral( "projectlayers" ) ).firstChildElement( QStringLiteral( "maplayer" ) );
  QCOMPARE( migratedLayer.attribute( QStringLiteral( "type" ) ), QStringLiteral( "annotation" ) );
  const QDomNodeList items = migratedLayer.firstChildElement( QStringLiteral( "items" ) ).elementsByTagName( QStringLiteral( "item" ) );
  QCOMPARE( items.size(), 1 );
  QCOMPARE( items.at( 0 ).toElement().attribute( QStringLiteral( "type" ) ), QStringLiteral( "kadas:pin" ) );
}

void TestKadasProjectMigration::migrateLegacyKadasItemLayer_translatesCircularSectorItem()
{
  // KadasCircularSectorItem stores N (center, radius, startAngle, stopAngle)
  // tuples; each tuple fans into one QgsAnnotationPolygonItem.
  QDomDocument doc;
  QDomElement root = doc.createElement( QStringLiteral( "qgis" ) );
  doc.appendChild( root );
  QDomElement projectLayersEl = doc.createElement( QStringLiteral( "projectlayers" ) );
  root.appendChild( projectLayersEl );

  QDomElement mapLayerEl = appendKadasItemLayer( doc, projectLayersEl, QStringLiteral( "sec_id" ), QStringLiteral( "Sectors" ), QStringLiteral( "EPSG:3857" ) );

  QDomElement itemEl = doc.createElement( QStringLiteral( "MapItem" ) );
  itemEl.setAttribute( QStringLiteral( "name" ), QStringLiteral( "KadasCircularSectorItem" ) );
  itemEl.setAttribute( QStringLiteral( "crs" ), QStringLiteral( "EPSG:3857" ) );
  itemEl.setAttribute( QStringLiteral( "format_version" ), QStringLiteral( "2" ) );
  itemEl.setAttribute( QStringLiteral( "outline_color" ), QStringLiteral( "#000000" ) );
  itemEl.setAttribute( QStringLiteral( "outline_width" ), QStringLiteral( "1" ) );
  itemEl.setAttribute( QStringLiteral( "outline_style" ), QString::number( static_cast<int>( Qt::SolidLine ) ) );
  itemEl.setAttribute( QStringLiteral( "fill_color" ), QStringLiteral( "#8000ff00" ) );
  itemEl.setAttribute( QStringLiteral( "fill_style" ), QString::number( static_cast<int>( Qt::SolidPattern ) ) );
  // Two sectors at distinct centers.
  itemEl.setAttribute( QStringLiteral( "centers" ), QStringLiteral( "0,0;100,100" ) );
  itemEl.setAttribute( QStringLiteral( "radii" ), QStringLiteral( "10;20" ) );
  itemEl.setAttribute( QStringLiteral( "start_angles" ), QStringLiteral( "0;0" ) );
  itemEl.setAttribute( QStringLiteral( "stop_angles" ), QStringLiteral( "1.5708;1.5708" ) );
  mapLayerEl.appendChild( itemEl );

  QStringList filesToAttach;
  QVERIFY( KadasProjectMigration::migrateProjectXml( QString(), doc, filesToAttach ) );

  const QDomElement migratedLayer = doc.documentElement().firstChildElement( QStringLiteral( "projectlayers" ) ).firstChildElement( QStringLiteral( "maplayer" ) );
  QCOMPARE( migratedLayer.attribute( QStringLiteral( "type" ) ), QStringLiteral( "annotation" ) );
  const QDomNodeList items = migratedLayer.firstChildElement( QStringLiteral( "items" ) ).elementsByTagName( QStringLiteral( "item" ) );
  QCOMPARE( items.size(), 2 );
  QCOMPARE( items.at( 0 ).toElement().attribute( QStringLiteral( "type" ) ), QStringLiteral( "polygon" ) );
  QCOMPARE( items.at( 1 ).toElement().attribute( QStringLiteral( "type" ) ), QStringLiteral( "polygon" ) );
}

void TestKadasProjectMigration::migrateLegacyKadasItemLayer_translatesGpxRouteItem()
{
  // KadasGpxRouteItem extends KadasLineItem with gpx_name/gpx_number
  // and label_font/label_color. Translates to KadasGpxRouteAnnotationItem
  // (type=kadas:gpxroute), one per MultiLineString part.
  QDomDocument doc;
  QDomElement root = doc.createElement( QStringLiteral( "qgis" ) );
  doc.appendChild( root );
  QDomElement projectLayersEl = doc.createElement( QStringLiteral( "projectlayers" ) );
  root.appendChild( projectLayersEl );

  QDomElement mapLayerEl = appendKadasItemLayer( doc, projectLayersEl, QStringLiteral( "route_id" ), QStringLiteral( "Routes" ), QStringLiteral( "EPSG:3857" ) );

  QDomElement itemEl = doc.createElement( QStringLiteral( "MapItem" ) );
  itemEl.setAttribute( QStringLiteral( "name" ), QStringLiteral( "KadasGpxRouteItem" ) );
  itemEl.setAttribute( QStringLiteral( "crs" ), QStringLiteral( "EPSG:3857" ) );
  itemEl.setAttribute( QStringLiteral( "format_version" ), QStringLiteral( "2" ) );
  itemEl.setAttribute( QStringLiteral( "outline_color" ), QStringLiteral( "#ff0000" ) );
  itemEl.setAttribute( QStringLiteral( "outline_width" ), QStringLiteral( "2" ) );
  itemEl.setAttribute( QStringLiteral( "outline_style" ), QString::number( static_cast<int>( Qt::SolidLine ) ) );
  itemEl.setAttribute( QStringLiteral( "fill_color" ), QStringLiteral( "#00000000" ) );
  itemEl.setAttribute( QStringLiteral( "fill_style" ), QString::number( static_cast<int>( Qt::SolidPattern ) ) );
  itemEl.setAttribute( QStringLiteral( "geometry" ), QStringLiteral( "MultiLineString ((0 0, 1 1, 2 0), (10 10, 11 11))" ) );
  itemEl.setAttribute( QStringLiteral( "gpx_name" ), QStringLiteral( "Route A" ) );
  itemEl.setAttribute( QStringLiteral( "gpx_number" ), QStringLiteral( "42" ) );
  mapLayerEl.appendChild( itemEl );

  QStringList filesToAttach;
  QVERIFY( KadasProjectMigration::migrateProjectXml( QString(), doc, filesToAttach ) );

  const QDomElement migratedLayer = doc.documentElement().firstChildElement( QStringLiteral( "projectlayers" ) ).firstChildElement( QStringLiteral( "maplayer" ) );
  QCOMPARE( migratedLayer.attribute( QStringLiteral( "type" ) ), QStringLiteral( "annotation" ) );
  const QDomNodeList items = migratedLayer.firstChildElement( QStringLiteral( "items" ) ).elementsByTagName( QStringLiteral( "item" ) );
  QCOMPARE( items.size(), 2 );
  QCOMPARE( items.at( 0 ).toElement().attribute( QStringLiteral( "type" ) ), QStringLiteral( "kadas:gpxroute" ) );
  QCOMPARE( items.at( 1 ).toElement().attribute( QStringLiteral( "type" ) ), QStringLiteral( "kadas:gpxroute" ) );
}

void TestKadasProjectMigration::migrateLegacyKadasItemLayer_translatesGpxWaypointItem()
{
  // KadasGpxWaypointItem extends KadasPointItem with gpx_name and
  // label_font/label_color. Translates to KadasGpxWaypointAnnotationItem
  // (type=kadas:gpxwaypoint).
  QDomDocument doc;
  QDomElement root = doc.createElement( QStringLiteral( "qgis" ) );
  doc.appendChild( root );
  QDomElement projectLayersEl = doc.createElement( QStringLiteral( "projectlayers" ) );
  root.appendChild( projectLayersEl );

  QDomElement mapLayerEl = appendKadasItemLayer( doc, projectLayersEl, QStringLiteral( "wp_id" ), QStringLiteral( "Waypoints" ), QStringLiteral( "EPSG:3857" ) );

  QDomElement itemEl = doc.createElement( QStringLiteral( "MapItem" ) );
  itemEl.setAttribute( QStringLiteral( "name" ), QStringLiteral( "KadasGpxWaypointItem" ) );
  itemEl.setAttribute( QStringLiteral( "crs" ), QStringLiteral( "EPSG:3857" ) );
  itemEl.setAttribute( QStringLiteral( "format_version" ), QStringLiteral( "2" ) );
  itemEl.setAttribute( QStringLiteral( "shape" ), QStringLiteral( "Circle" ) );
  itemEl.setAttribute( QStringLiteral( "size" ), QStringLiteral( "6" ) );
  itemEl.setAttribute( QStringLiteral( "stroke_color" ), QStringLiteral( "#ff0000" ) );
  itemEl.setAttribute( QStringLiteral( "stroke_width" ), QStringLiteral( "2" ) );
  itemEl.setAttribute( QStringLiteral( "fill_color" ), QStringLiteral( "#ffff00" ) );
  itemEl.setAttribute( QStringLiteral( "geometry" ), QStringLiteral( "POINT(500 600)" ) );
  itemEl.setAttribute( QStringLiteral( "gpx_name" ), QStringLiteral( "WP A" ) );
  mapLayerEl.appendChild( itemEl );

  QStringList filesToAttach;
  QVERIFY( KadasProjectMigration::migrateProjectXml( QString(), doc, filesToAttach ) );

  const QDomElement migratedLayer = doc.documentElement().firstChildElement( QStringLiteral( "projectlayers" ) ).firstChildElement( QStringLiteral( "maplayer" ) );
  QCOMPARE( migratedLayer.attribute( QStringLiteral( "type" ) ), QStringLiteral( "annotation" ) );
  const QDomNodeList items = migratedLayer.firstChildElement( QStringLiteral( "items" ) ).elementsByTagName( QStringLiteral( "item" ) );
  QCOMPARE( items.size(), 1 );
  QCOMPARE( items.at( 0 ).toElement().attribute( QStringLiteral( "type" ) ), QStringLiteral( "kadas:gpxwaypoint" ) );
}

void TestKadasProjectMigration::migrateLegacyKadasItemLayer_leavesV1FormatAlone()
{
  // A `KadasItemLayer` whose only `<MapItem>` is in the legacy v1
  // (JSON-in-CDATA) format must be left as a plugin layer (the
  // XML rewriter only handles v2; v1 projects will fail to load).
  QDomDocument doc;
  QDomElement root = doc.createElement( QStringLiteral( "qgis" ) );
  doc.appendChild( root );
  QDomElement projectLayersEl = doc.createElement( QStringLiteral( "projectlayers" ) );
  root.appendChild( projectLayersEl );

  QDomElement mapLayerEl = appendKadasItemLayer( doc, projectLayersEl, QStringLiteral( "v1_id" ), QStringLiteral( "V1" ), QStringLiteral( "EPSG:3857" ) );
  QDomElement itemEl = doc.createElement( QStringLiteral( "MapItem" ) );
  itemEl.setAttribute( QStringLiteral( "name" ), QStringLiteral( "KadasPointItem" ) );
  itemEl.setAttribute( QStringLiteral( "crs" ), QStringLiteral( "EPSG:3857" ) );
  // No format_version attribute → v1 by default.
  itemEl.appendChild( doc.createCDATASection( QStringLiteral( "{\"props\":{},\"state\":{}}" ) ) );
  mapLayerEl.appendChild( itemEl );

  QStringList filesToAttach;
  // Return value is false when nothing was rewritten — but the function
  // is allowed to be called regardless; what matters is the layer block
  // is untouched.
  KadasProjectMigration::migrateProjectXml( QString(), doc, filesToAttach );

  const QDomElement preserved = doc.documentElement().firstChildElement( QStringLiteral( "projectlayers" ) ).firstChildElement( QStringLiteral( "maplayer" ) );
  QCOMPARE( preserved.attribute( QStringLiteral( "type" ) ), QStringLiteral( "plugin" ) );
  QCOMPARE( preserved.attribute( QStringLiteral( "name" ) ), QStringLiteral( "KadasItemLayer" ) );
}

void TestKadasProjectMigration::migrateLegacyKadasItemLayer_leavesUnknownItemTypeAlone()
{
  // A layer mixing a translatable Point item with a not-yet-translatable
  // item type (e.g. KadasLineItem before slice 7r lands) must be left
  // untouched: all-or-nothing keeps the post-load fallback authoritative.
  QDomDocument doc;
  QDomElement root = doc.createElement( QStringLiteral( "qgis" ) );
  doc.appendChild( root );
  QDomElement projectLayersEl = doc.createElement( QStringLiteral( "projectlayers" ) );
  root.appendChild( projectLayersEl );

  QDomElement mapLayerEl = appendKadasItemLayer( doc, projectLayersEl, QStringLiteral( "mixed_id" ), QStringLiteral( "Mixed" ), QStringLiteral( "EPSG:3857" ) );
  appendV2PointMapItem( doc, mapLayerEl, 0, 0, QStringLiteral( "EPSG:3857" ) );
  QDomElement lineEl = doc.createElement( QStringLiteral( "MapItem" ) );
  lineEl.setAttribute( QStringLiteral( "name" ), QStringLiteral( "KadasUnknownFutureItem" ) );
  lineEl.setAttribute( QStringLiteral( "format_version" ), QStringLiteral( "2" ) );
  mapLayerEl.appendChild( lineEl );

  QStringList filesToAttach;
  KadasProjectMigration::migrateProjectXml( QString(), doc, filesToAttach );

  const QDomElement preserved = doc.documentElement().firstChildElement( QStringLiteral( "projectlayers" ) ).firstChildElement( QStringLiteral( "maplayer" ) );
  QCOMPARE( preserved.attribute( QStringLiteral( "type" ) ), QStringLiteral( "plugin" ) );
  QCOMPARE( preserved.attribute( QStringLiteral( "name" ) ), QStringLiteral( "KadasItemLayer" ) );
}

void TestKadasProjectMigration::migrateLegacyPluginGridLayers_translatesBullseyeAndGuideGrid()
{
  // Legacy bullseye / guide-grid plugin layers (config carried as XML
  // attributes on the `<maplayer>` element) must be rewritten into
  // QgsAnnotationLayer blocks carrying the `kadas/annotation-type`
  // customProperty marker + per-type config keys, so the post-load
  // promotion pass can rebuild the Kadas subclasses. The plugin layer
  // types no longer exist; without this rewrite both layers are dropped
  // as unknown plugin layers at load.
  QDomDocument doc;
  QDomElement root = doc.createElement( QStringLiteral( "qgis" ) );
  doc.appendChild( root );
  QDomElement projectLayersEl = doc.createElement( QStringLiteral( "projectlayers" ) );
  root.appendChild( projectLayersEl );

  const auto appendIdAndName = [&doc]( QDomElement &el, const QString &id, const QString &name ) {
    QDomElement idEl = doc.createElement( QStringLiteral( "id" ) );
    idEl.appendChild( doc.createTextNode( id ) );
    el.appendChild( idEl );
    QDomElement nameEl = doc.createElement( QStringLiteral( "layername" ) );
    nameEl.appendChild( doc.createTextNode( name ) );
    el.appendChild( nameEl );
  };

  // Bullseye — attribute layout mirrors a real Kadas 2.x project save.
  QDomElement bullseyeEl = doc.createElement( QStringLiteral( "maplayer" ) );
  bullseyeEl.setAttribute( QStringLiteral( "type" ), QStringLiteral( "plugin" ) );
  bullseyeEl.setAttribute( QStringLiteral( "name" ), QStringLiteral( "bullseye" ) );
  bullseyeEl.setAttribute( QStringLiteral( "crs" ), QStringLiteral( "EPSG:2056" ) );
  bullseyeEl.setAttribute( QStringLiteral( "x" ), QStringLiteral( "3122810.86789" ) );
  bullseyeEl.setAttribute( QStringLiteral( "y" ), QStringLiteral( "1144589.83167" ) );
  bullseyeEl.setAttribute( QStringLiteral( "rings" ), QStringLiteral( "3" ) );
  bullseyeEl.setAttribute( QStringLiteral( "axes" ), QStringLiteral( "45" ) );
  bullseyeEl.setAttribute( QStringLiteral( "interval" ), QStringLiteral( "28.45" ) );
  bullseyeEl.setAttribute( QStringLiteral( "intervalUnit" ), QStringLiteral( "nautical miles" ) );
  bullseyeEl.setAttribute( QStringLiteral( "fontSize" ), QStringLiteral( "21" ) );
  bullseyeEl.setAttribute( QStringLiteral( "lineWidth" ), QStringLiteral( "7" ) );
  bullseyeEl.setAttribute( QStringLiteral( "color" ), QStringLiteral( "253,191,111,255" ) );
  bullseyeEl.setAttribute( QStringLiteral( "labelAxes" ), QStringLiteral( "1" ) );
  bullseyeEl.setAttribute( QStringLiteral( "labelQuadrants" ), QStringLiteral( "1" ) );
  bullseyeEl.setAttribute( QStringLiteral( "labelRings" ), QStringLiteral( "1" ) );
  bullseyeEl.setAttribute( QStringLiteral( "transparency" ), QStringLiteral( "0" ) );
  appendIdAndName( bullseyeEl, QStringLiteral( "Bullseye_layer_id" ), QStringLiteral( "Bullseye" ) );
  projectLayersEl.appendChild( bullseyeEl );

  // Guide grid.
  QDomElement gridEl = doc.createElement( QStringLiteral( "maplayer" ) );
  gridEl.setAttribute( QStringLiteral( "type" ), QStringLiteral( "plugin" ) );
  gridEl.setAttribute( QStringLiteral( "name" ), QStringLiteral( "guide_grid" ) );
  gridEl.setAttribute( QStringLiteral( "crs" ), QStringLiteral( "EPSG:2056" ) );
  gridEl.setAttribute( QStringLiteral( "xmin" ), QStringLiteral( "2364310.87" ) );
  gridEl.setAttribute( QStringLiteral( "ymin" ), QStringLiteral( "899839.79" ) );
  gridEl.setAttribute( QStringLiteral( "xmax" ), QStringLiteral( "2870310.93" ) );
  gridEl.setAttribute( QStringLiteral( "ymax" ), QStringLiteral( "1405839.85" ) );
  gridEl.setAttribute( QStringLiteral( "cols" ), QStringLiteral( "6" ) );
  gridEl.setAttribute( QStringLiteral( "rows" ), QStringLiteral( "10" ) );
  gridEl.setAttribute( QStringLiteral( "colChar" ), QStringLiteral( "A" ) );
  gridEl.setAttribute( QStringLiteral( "rowChar" ), QStringLiteral( "1" ) );
  gridEl.setAttribute( QStringLiteral( "fontSize" ), QStringLiteral( "34" ) );
  gridEl.setAttribute( QStringLiteral( "lineWidth" ), QStringLiteral( "5" ) );
  gridEl.setAttribute( QStringLiteral( "color" ), QStringLiteral( "51,160,44,255" ) );
  gridEl.setAttribute( QStringLiteral( "quadrantLabeling" ), QStringLiteral( "0" ) );
  gridEl.setAttribute( QStringLiteral( "labelingPos" ), QStringLiteral( "0" ) );
  gridEl.setAttribute( QStringLiteral( "colSizeLocked" ), QStringLiteral( "0" ) );
  gridEl.setAttribute( QStringLiteral( "rowSizeLocked" ), QStringLiteral( "0" ) );
  appendIdAndName( gridEl, QStringLiteral( "GuideGrid_layer_id" ), QStringLiteral( "Grille de conduite" ) );
  projectLayersEl.appendChild( gridEl );

  // Layer-tree entries carrying the defunct plugin providerKey.
  QDomElement treeRoot = doc.createElement( QStringLiteral( "layer-tree-group" ) );
  root.appendChild( treeRoot );
  const auto appendTreeLayer = [&doc, &treeRoot]( const QString &id, const QString &providerKey ) {
    QDomElement treeLayer = doc.createElement( QStringLiteral( "layer-tree-layer" ) );
    treeLayer.setAttribute( QStringLiteral( "id" ), id );
    treeLayer.setAttribute( QStringLiteral( "providerKey" ), providerKey );
    treeRoot.appendChild( treeLayer );
  };
  appendTreeLayer( QStringLiteral( "Bullseye_layer_id" ), QStringLiteral( "bullseye" ) );
  appendTreeLayer( QStringLiteral( "GuideGrid_layer_id" ), QStringLiteral( "guide_grid" ) );

  QStringList filesToAttach;
  QVERIFY( KadasProjectMigration::migrateProjectXml( QString(), doc, filesToAttach ) );

  const QDomElement migratedProjectLayers = doc.documentElement().firstChildElement( QStringLiteral( "projectlayers" ) );
  int annotationLayers = 0;
  for ( QDomElement el = migratedProjectLayers.firstChildElement( QStringLiteral( "maplayer" ) ); !el.isNull(); el = el.nextSiblingElement( QStringLiteral( "maplayer" ) ) )
  {
    QCOMPARE( el.attribute( QStringLiteral( "type" ) ), QStringLiteral( "annotation" ) );
    ++annotationLayers;

    const QString id = el.firstChildElement( QStringLiteral( "id" ) ).text();
    QString xml;
    QTextStream stream( &xml );
    el.save( stream, 0 );
    if ( id == QLatin1String( "Bullseye_layer_id" ) )
    {
      QCOMPARE( el.firstChildElement( QStringLiteral( "layername" ) ).text(), QStringLiteral( "Bullseye" ) );
      QVERIFY( xml.contains( QStringLiteral( "kadas/annotation-type" ) ) );
      QVERIFY( xml.contains( QStringLiteral( "kadas/bullseye/rings" ) ) );
      QVERIFY( xml.contains( QStringLiteral( "nautical miles" ) ) );
      QVERIFY( xml.contains( QStringLiteral( "253,191,111,255" ) ) );
    }
    else
    {
      QCOMPARE( id, QStringLiteral( "GuideGrid_layer_id" ) );
      QCOMPARE( el.firstChildElement( QStringLiteral( "layername" ) ).text(), QStringLiteral( "Grille de conduite" ) );
      QVERIFY( xml.contains( QStringLiteral( "kadas/guidegrid/cols" ) ) );
      QVERIFY( xml.contains( QStringLiteral( "51,160,44,255" ) ) );
    }
  }
  QCOMPARE( annotationLayers, 2 );

  // providerKey must be cleared so QGIS resolves the layers through the
  // (now annotation) maplayer blocks.
  const QDomNodeList treeLayers = doc.documentElement().elementsByTagName( QStringLiteral( "layer-tree-layer" ) );
  for ( int i = 0; i < treeLayers.size(); ++i )
    QVERIFY( treeLayers.at( i ).toElement().attribute( QStringLiteral( "providerKey" ) ).isEmpty() );
}


QTEST_MAIN( TestKadasProjectMigration )
#include "testkadasprojectmigration.moc"
