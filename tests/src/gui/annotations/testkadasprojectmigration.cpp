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
#include <QStandardPaths>
#include <QStringList>
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
    void migrateLegacyKadasItemLayer_leavesV1FormatAlone();
    void migrateLegacyKadasItemLayer_leavesUnknownItemTypeAlone();

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

void TestKadasProjectMigration::migrateLegacyKadasItemLayer_leavesV1FormatAlone()
{
  // A `KadasItemLayer` whose only `<MapItem>` is in the legacy v1
  // (JSON-in-CDATA) format must be left as a plugin layer for the
  // post-load `KadasItemLayerMigration` fallback to handle.
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
  lineEl.setAttribute( QStringLiteral( "name" ), QStringLiteral( "KadasLineItem" ) );
  lineEl.setAttribute( QStringLiteral( "format_version" ), QStringLiteral( "2" ) );
  mapLayerEl.appendChild( lineEl );

  QStringList filesToAttach;
  KadasProjectMigration::migrateProjectXml( QString(), doc, filesToAttach );

  const QDomElement preserved = doc.documentElement().firstChildElement( QStringLiteral( "projectlayers" ) ).firstChildElement( QStringLiteral( "maplayer" ) );
  QCOMPARE( preserved.attribute( QStringLiteral( "type" ) ), QStringLiteral( "plugin" ) );
  QCOMPARE( preserved.attribute( QStringLiteral( "name" ) ), QStringLiteral( "KadasItemLayer" ) );
}


QTEST_MAIN( TestKadasProjectMigration )
#include "testkadasprojectmigration.moc"
