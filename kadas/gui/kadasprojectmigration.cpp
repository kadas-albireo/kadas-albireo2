/***************************************************************************
    kadasprojectmigration.cpp
    -------------------------
    copyright            : (C) 2019 by Sandro Mani
    email                : smani at sourcepole dot ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <QApplication>
#include <QDir>
#include <QDomDocument>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QScreen>
#include <QFile>
#include <QMap>
#include <QSizeF>
#include <QTemporaryDir>

#include <cmath>

#include <qgis/qgsannotationlayer.h>
#include <qgis/qgscircularstring.h>
#include <qgis/qgslinestring.h>
#include <qgis/qgslogger.h>
#include <qgis/qgsmessagelog.h>
#include <qgis/qgspoint.h>
#include <qgis/qgspolygon.h>
#include <qgis/qgsproject.h>
#include <qgis/qgssymbollayerutils.h>
#include <qgis/qgsunittypes.h>

#include "kadas/core/kadas.h"
#include "kadas/gui/annotationitems/kadasannotationlayerhelpers.h"
#include "kadas/gui/annotationitems/kadasmilxannotationitem.h"
#include "kadas/gui/annotationitems/kadasmilxlayersettings.h"
#include "kadas/gui/kadasprojectmigration.h"
#include "kadas/gui/milx/kadasmilxclient.h"

#include <qgis/qgsarchive.h>
#include <qgis/qgsziputils.h>


QString KadasProjectMigration::migrateProject( const QString &fileName, QStringList &filesToAttach )
{
  // Kadas projects are typically stored as `.qgz` zip archives bundling
  // a `.qgs` XML document and any attachments. Handle both cases:
  //  - `.qgz`: unzip into a temp dir, mutate the embedded `.qgs`, repack
  //    into a fresh temp `.qgz` (preserving the attachments) and return
  //    its path.
  //  - `.qgs`: read XML directly, mutate, write to a temp file, return
  //    its path.
  // If the file does not exist, fails to parse, or no migration was
  // necessary, the original path is returned untouched.
  if ( QgsZipUtils::isZipFile( fileName ) )
  {
    auto archive = std::make_unique<QgsArchive>();
    if ( !archive->unzip( fileName ) )
    {
      QgsDebugMsgLevel( "Failed to unzip project archive", 2 );
      return fileName;
    }
    // Find the `.qgs` document inside the archive. Bundles always have
    // exactly one.
    QString qgsPath;
    const QStringList archiveFiles = archive->files();
    for ( const QString &f : archiveFiles )
    {
      if ( f.endsWith( QLatin1String( ".qgs" ), Qt::CaseInsensitive ) )
      {
        qgsPath = f;
        break;
      }
    }
    if ( qgsPath.isEmpty() )
      return fileName;

    QFile qgsFile( qgsPath );
    if ( !qgsFile.open( QIODevice::ReadOnly ) )
      return fileName;
    QDomDocument doc;
    if ( !doc.setContent( &qgsFile ) )
    {
      qgsFile.close();
      return fileName;
    }
    qgsFile.close();

    const QString basedir = QFileInfo( fileName ).path();
    if ( !migrateProjectXml( basedir, doc, filesToAttach ) )
      return fileName;

    // Write the migrated XML back over the unzipped `.qgs`.
    if ( !qgsFile.open( QIODevice::WriteOnly | QIODevice::Truncate ) )
      return fileName;
    qgsFile.write( doc.toString().toUtf8() );
    qgsFile.close();

    // Repack into a fresh temp dir under a never-used name: zip() ends with
    // a rename, which on Windows fails onto a name we created and removed
    // ourselves (delete-pending), silently dropping the migration.
    auto outDir = std::make_unique<QTemporaryDir>();
    outDir->setAutoRemove( false );
    if ( !outDir->isValid() )
    {
      QgsMessageLog::
        logMessage( QStringLiteral( "Kadas project migration: could not create a temp dir for the migrated project; opening the original instead" ), QStringLiteral( "Kadas" ), Qgis::MessageLevel::Warning );
      return fileName;
    }
    const QString outPath = QDir( outDir->path() ).filePath( QStringLiteral( "kadas-migrated.qgz" ) );
    if ( !archive->zip( outPath ) )
    {
      QgsMessageLog::
        logMessage( QStringLiteral( "Kadas project migration: failed to write the migrated project archive '%1'; opening the original instead" ).arg( outPath ), QStringLiteral( "Kadas" ), Qgis::MessageLevel::Warning );
      return fileName;
    }
    outDir.release(); // migrated archive must outlive this call
    return outPath;
  }

  QFile file( fileName );
  if ( !file.open( QIODevice::ReadOnly ) )
  {
    return fileName;
  }

  QDomDocument doc;
  if ( !doc.setContent( &file ) )
  {
    QgsDebugMsgLevel( "Failed to parse project", 2 );
    return fileName;
  }
  QString basedir = QFileInfo( fileName ).path();

  if ( migrateProjectXml( basedir, doc, filesToAttach ) )
  {
    QTemporaryFile tempFile;
    tempFile.setAutoRemove( false );
    if ( tempFile.open() )
    {
      tempFile.write( doc.toString().toUtf8() );
      tempFile.close();
      return tempFile.fileName();
    }
  }

  return fileName;
}

bool KadasProjectMigration::migrateProjectXml( const QString &basedir, QDomDocument &doc, QStringList &filesToAttach )
{
  QDomElement root = doc.documentElement();
  if ( root.tagName() != "qgis" )
  {
    QgsDebugMsgLevel( "Invalid project (incorrect root tag name)", 2 );
    return false;
  }

  Q_UNUSED( basedir );
  Q_UNUSED( filesToAttach );

  if ( root.attribute( "version" ) == "2.15.2-KADAS" )
  {
    // Kadas 1.x projects are no longer supported; leave the document
    // untouched so QGIS reports a normal load failure.
    return false;
  }

  // Kadas Albireo 2.x intermediate projects shipped with `KadasMilxLayer`
  // plugin layers containing `KadasMilxItem` children. The plugin-layer
  // type and item factory have since been removed in favour of
  // `QgsAnnotationLayer` + `KadasMilxAnnotationItem`. Rewrite those
  // blocks before QGIS opens the project so the symbols survive.
  bool changed = migrateLegacyMilxLayers( doc, root );
  // Same for legacy `KadasItemLayer` plugin layers carrying redlining
  // `KadasMapItem` children: translate each MapItem into the matching
  // `QgsAnnotationItem` subclass at XML level. Layers whose items cannot
  // all be translated by this pass are left as plugin layers and will
  // fail to load (the post-load migration fallback has been removed).
  changed = migrateLegacyKadasItemLayers( doc, root ) || changed;
  // Legacy bullseye / guide-grid plugin layers: the plugin layer types
  // were dropped when both became QgsAnnotationLayer subclasses; rewrite
  // the plugin blocks into annotation-layer blocks carrying the
  // `kadas/...` customProperties so the post-load promotion pass can
  // rebuild the Kadas subclasses.
  changed = migrateLegacyPluginGridLayers( doc, root ) || changed;
  return changed;
}

bool KadasProjectMigration::migrateLegacyMilxLayers( QDomDocument &doc, QDomElement &root )
{
  QDomElement projectLayersEl = root.firstChildElement( "projectlayers" );
  if ( projectLayersEl.isNull() )
    return false;

  // Snapshot direct `<maplayer>` children of `<projectlayers>`. Using
  // `elementsByTagName` would also pick up nested `<maplayer>` blocks
  // inside `<originalStyle>` etc., which we must not touch.
  QList<QDomElement> milxMapLayers;
  for ( QDomNode n = projectLayersEl.firstChild(); !n.isNull(); n = n.nextSibling() )
  {
    QDomElement el = n.toElement();
    if ( el.tagName() == QLatin1String( "maplayer" ) && el.attribute( QStringLiteral( "type" ) ) == QLatin1String( "plugin" ) && el.attribute( QStringLiteral( "name" ) ) == QLatin1String( "KadasMilxLayer" ) )
    {
      milxMapLayers.append( el );
    }
  }
  if ( milxMapLayers.isEmpty() )
    return false;

  QgsDebugMsgLevel( QStringLiteral( "Migrating %1 legacy KadasMilxLayer plugin layer(s) to QgsAnnotationLayer" ).arg( milxMapLayers.size() ), 1 );

  for ( QDomElement &mapLayerEl : milxMapLayers )
  {
    const QString layerName = mapLayerEl.firstChildElement( "layername" ).text();

    QgsAnnotationLayer::LayerOptions options( QgsProject::instance()->transformContext() );
    auto annoLayer = std::make_unique<QgsAnnotationLayer>( layerName, options );
    annoLayer->setCrs( QgsCoordinateReferenceSystem( "EPSG:4326" ) );

    // Translate per-layer MilX symbol-settings overrides (carried as XML
    // attributes on the legacy `<maplayer>` element) into the
    // customProperty namespace used by KadasMilxLayerSettings.
    if ( mapLayerEl.hasAttribute( QStringLiteral( "milx_override_symbol_settings" ) ) )
    {
      KadasMilxSymbolSettings settings = KadasMilxClient::globalSymbolSettings();
      settings.symbolSize = mapLayerEl.attribute( QStringLiteral( "milx_symbol_size" ), QString::number( settings.symbolSize ) ).toInt();
      settings.lineWidth = mapLayerEl.attribute( QStringLiteral( "milx_line_width" ), QString::number( settings.lineWidth ) ).toInt();
      settings.workMode = static_cast<KadasMilxSymbolSettings::WorkMode>( mapLayerEl.attribute( QStringLiteral( "milx_work_mode" ), QString::number( static_cast<int>( settings.workMode ) ) ).toInt() );
      settings.leaderLineWidth = mapLayerEl.attribute( QStringLiteral( "milx_leader_line_width" ), QString::number( settings.leaderLineWidth ) ).toInt();
      settings.leaderLineColor = QColor( mapLayerEl.attribute( QStringLiteral( "milx_leader_line_color" ), QColor( settings.leaderLineColor ).name() ) );
      KadasMilxLayerSettings::setLayerSettings( annoLayer.get(), settings );
      KadasMilxLayerSettings::setOverrideEnabled( annoLayer.get(), mapLayerEl.attribute( QStringLiteral( "milx_override_symbol_settings" ) ).toInt() != 0 );
    }

    // Convert each `<MapItem name="KadasMilxItem">` JSON-in-CDATA payload
    // into a populated `KadasMilxAnnotationItem`. MilX is fixed to
    // EPSG:4326 in both the legacy and the new format, so no CRS
    // transform is needed.
    int migratedCount = 0;
    for ( QDomNode itemNode = mapLayerEl.firstChild(); !itemNode.isNull(); itemNode = itemNode.nextSibling() )
    {
      QDomElement itemEl = itemNode.toElement();
      if ( itemEl.isNull() )
        continue;
      if ( itemEl.tagName() != QLatin1String( "MapItem" ) || itemEl.attribute( QStringLiteral( "name" ) ) != QLatin1String( "KadasMilxItem" ) )
        continue;

      // JSON is UTF-8 by spec. `toLocal8Bit()` would corrupt non-ASCII
      // payloads on Windows (CP1252) — e.g. accented `militaryName`
      // values — making `fromJson` return a null object so the item is
      // silently dropped during migration.
      const QJsonObject data = QJsonDocument::fromJson( itemEl.firstChild().toCDATASection().data().toUtf8() ).object();
      const QJsonObject props = data.value( QStringLiteral( "props" ) ).toObject();
      const QString mssString = props.value( QStringLiteral( "mssString" ) ).toString();
      if ( mssString.isEmpty() )
        continue;

      auto *anno = new KadasMilxAnnotationItem();
      anno->setMssString( mssString );
      anno->setMilitaryName( props.value( QStringLiteral( "militaryName" ) ).toString() );
      anno->setSymbolType( props.value( QStringLiteral( "symbolType" ) ).toString() );
      anno->setMinNumPoints( std::max( 1, props.value( QStringLiteral( "minNPoints" ) ).toInt( 1 ) ) );
      anno->setHasVariablePoints( props.value( QStringLiteral( "hasVariablePoints" ) ).toBool() );

      const QJsonObject state = data.value( QStringLiteral( "state" ) ).toObject();

      QList<QgsPointXY> pts;
      const QJsonArray ptsArr = state.value( QStringLiteral( "points" ) ).toArray();
      pts.reserve( ptsArr.size() );
      for ( const QJsonValue &v : ptsArr )
      {
        const QJsonArray p = v.toArray();
        pts.append( QgsPointXY( p.at( 0 ).toDouble(), p.at( 1 ).toDouble() ) );
      }
      anno->setPoints( pts );

      QList<int> ctrl;
      const QJsonArray ctrlArr = state.value( QStringLiteral( "controlPoints" ) ).toArray();
      for ( const QJsonValue &v : ctrlArr )
        ctrl.append( v.toInt() );
      anno->setControlPoints( ctrl );

      QMap<KadasMilxAttrType, double> attrs;
      const QJsonArray attrsArr = state.value( QStringLiteral( "attributes" ) ).toArray();
      for ( const QJsonValue &v : attrsArr )
      {
        const QJsonArray a = v.toArray();
        attrs.insert( static_cast<KadasMilxAttrType>( a.at( 0 ).toInt() ), a.at( 1 ).toDouble() );
      }
      anno->setAttributes( attrs );

      QMap<KadasMilxAttrType, QgsPointXY> attrPts;
      const QJsonArray attrPtsArr = state.value( QStringLiteral( "attributePoints" ) ).toArray();
      for ( const QJsonValue &v : attrPtsArr )
      {
        const QJsonArray ap = v.toArray();
        const QJsonArray pt = ap.at( 1 ).toArray();
        attrPts.insert( static_cast<KadasMilxAttrType>( ap.at( 0 ).toInt() ), QgsPointXY( pt.at( 0 ).toDouble(), pt.at( 1 ).toDouble() ) );
      }
      anno->setAttributePoints( attrPts );

      const QJsonArray off = state.value( QStringLiteral( "userOffset" ) ).toArray();
      anno->setUserOffset( QPoint( off.at( 0 ).toDouble(), off.at( 1 ).toDouble() ) );

      const QString itemId = annoLayer->addItem( anno );
      Q_UNUSED( itemId );
      ++migratedCount;
    }
    QgsDebugMsgLevel( QStringLiteral( "Migrated %1 MilX item(s) from layer '%2'" ).arg( migratedCount ).arg( layerName ), 2 );

    QDomElement newMapLayerEl = doc.createElement( "maplayer" );
    QgsReadWriteContext context;
    annoLayer->writeLayerXml( newMapLayerEl, doc, context );
    // Preserve the original layer id and name so that layer-tree-layer,
    // layerorder, custom-order and similar references continue to
    // resolve. `writeLayerXml` already wrote freshly generated `<id>`
    // and `<layername>` children, so replace those rather than append
    // duplicates (QGIS picks the first match).
    const QString originalId = mapLayerEl.firstChildElement( "id" ).text();
    const QString originalName = mapLayerEl.firstChildElement( "layername" ).text();
    QDomElement existingId = newMapLayerEl.firstChildElement( "id" );
    if ( !existingId.isNull() )
    {
      QDomElement replacementId = doc.createElement( "id" );
      replacementId.appendChild( doc.createTextNode( originalId ) );
      newMapLayerEl.replaceChild( replacementId, existingId );
    }
    QDomElement existingName = newMapLayerEl.firstChildElement( "layername" );
    if ( !existingName.isNull() )
    {
      QDomElement replacementName = doc.createElement( "layername" );
      replacementName.appendChild( doc.createTextNode( originalName ) );
      newMapLayerEl.replaceChild( replacementName, existingName );
    }

    projectLayersEl.replaceChild( newMapLayerEl, mapLayerEl );
  }

  // The layer-tree-layer entries reference the layer by id and carry
  // `providerKey="KadasMilxLayer"` from the legacy format. The plugin
  // provider key no longer exists; clear it so QGIS resolves the layer
  // through the projectlayers maplayer block (now an annotation layer).
  QDomNodeList treeLayers = root.elementsByTagName( QStringLiteral( "layer-tree-layer" ) );
  for ( int i = 0, n = treeLayers.size(); i < n; ++i )
  {
    QDomElement el = treeLayers.at( i ).toElement();
    if ( el.attribute( QStringLiteral( "providerKey" ) ) == QLatin1String( "KadasMilxLayer" ) )
      el.setAttribute( QStringLiteral( "providerKey" ), QString() );
  }

  return true;
}

bool KadasProjectMigration::migrateLegacyPluginGridLayers( QDomDocument &doc, QDomElement &root )
{
  QDomElement projectLayersEl = root.firstChildElement( "projectlayers" );
  if ( projectLayersEl.isNull() )
    return false;

  // Snapshot direct `<maplayer>` children of `<projectlayers>` (see
  // migrateLegacyMilxLayers for why elementsByTagName must not be used).
  QList<QDomElement> gridMapLayers;
  for ( QDomNode n = projectLayersEl.firstChild(); !n.isNull(); n = n.nextSibling() )
  {
    QDomElement el = n.toElement();
    if ( el.tagName() != QLatin1String( "maplayer" ) || el.attribute( QStringLiteral( "type" ) ) != QLatin1String( "plugin" ) )
      continue;
    const QString pluginType = el.attribute( QStringLiteral( "name" ) );
    if ( pluginType == QLatin1String( "bullseye" ) || pluginType == QLatin1String( "guide_grid" ) )
      gridMapLayers.append( el );
  }
  if ( gridMapLayers.isEmpty() )
    return false;

  QgsDebugMsgLevel( QStringLiteral( "Migrating %1 legacy bullseye/guide_grid plugin layer(s) to QgsAnnotationLayer" ).arg( gridMapLayers.size() ), 1 );

  for ( QDomElement &mapLayerEl : gridMapLayers )
  {
    const QString pluginType = mapLayerEl.attribute( QStringLiteral( "name" ) );
    const QString layerName = mapLayerEl.firstChildElement( "layername" ).text();

    QgsAnnotationLayer::LayerOptions options( QgsProject::instance()->transformContext() );
    auto annoLayer = std::make_unique<QgsAnnotationLayer>( layerName, options );

    QgsCoordinateReferenceSystem crs;
    const QDomElement srsEl = mapLayerEl.firstChildElement( QStringLiteral( "srs" ) );
    if ( !srsEl.isNull() )
      crs.readXml( srsEl );
    if ( !crs.isValid() )
      crs = QgsCoordinateReferenceSystem( mapLayerEl.attribute( QStringLiteral( "crs" ) ) );
    annoLayer->setCrs( crs );
    annoLayer->setOpacity( 1. - mapLayerEl.attribute( QStringLiteral( "transparency" ), QStringLiteral( "0" ) ).toDouble() / 100. );

    // Translate the legacy config (carried as XML attributes on the
    // `<maplayer>` element) into the customProperty namespace read by
    // KadasBullseyeLayer / KadasGuideGridLayer after promotion.
    if ( pluginType == QLatin1String( "bullseye" ) )
    {
      annoLayer->setCustomProperty( QStringLiteral( "kadas/annotation-type" ), QStringLiteral( "bullseye" ) );
      annoLayer->setCustomProperty( QStringLiteral( "kadas/bullseye/x" ), mapLayerEl.attribute( QStringLiteral( "x" ) ).toDouble() );
      annoLayer->setCustomProperty( QStringLiteral( "kadas/bullseye/y" ), mapLayerEl.attribute( QStringLiteral( "y" ) ).toDouble() );
      annoLayer->setCustomProperty( QStringLiteral( "kadas/bullseye/rings" ), mapLayerEl.attribute( QStringLiteral( "rings" ) ).toInt() );
      annoLayer->setCustomProperty( QStringLiteral( "kadas/bullseye/axes" ), mapLayerEl.attribute( QStringLiteral( "axes" ) ).toDouble() );
      annoLayer->setCustomProperty( QStringLiteral( "kadas/bullseye/interval" ), mapLayerEl.attribute( QStringLiteral( "interval" ) ).toDouble() );
      annoLayer->setCustomProperty( QStringLiteral( "kadas/bullseye/intervalUnit" ), mapLayerEl.attribute( QStringLiteral( "intervalUnit" ), QStringLiteral( "nautical miles" ) ) );
      annoLayer->setCustomProperty( QStringLiteral( "kadas/bullseye/fontSize" ), mapLayerEl.attribute( QStringLiteral( "fontSize" ), QStringLiteral( "10" ) ).toInt() );
      annoLayer->setCustomProperty( QStringLiteral( "kadas/bullseye/lineWidth" ), mapLayerEl.attribute( QStringLiteral( "lineWidth" ), QStringLiteral( "1" ) ).toInt() );
      annoLayer->setCustomProperty( QStringLiteral( "kadas/bullseye/color" ), mapLayerEl.attribute( QStringLiteral( "color" ), QStringLiteral( "0,0,255,255" ) ) );
      annoLayer->setCustomProperty( QStringLiteral( "kadas/bullseye/labelAxes" ), mapLayerEl.attribute( QStringLiteral( "labelAxes" ) ) == QLatin1String( "1" ) );
      annoLayer->setCustomProperty( QStringLiteral( "kadas/bullseye/labelQuadrants" ), mapLayerEl.attribute( QStringLiteral( "labelQuadrants" ) ) == QLatin1String( "1" ) );
      annoLayer->setCustomProperty( QStringLiteral( "kadas/bullseye/labelRings" ), mapLayerEl.attribute( QStringLiteral( "labelRings" ) ) == QLatin1String( "1" ) );
    }
    else // guide_grid
    {
      annoLayer->setCustomProperty( QStringLiteral( "kadas/annotation-type" ), QStringLiteral( "guidegrid" ) );
      annoLayer->setCustomProperty( QStringLiteral( "kadas/guidegrid/xmin" ), mapLayerEl.attribute( QStringLiteral( "xmin" ) ).toDouble() );
      annoLayer->setCustomProperty( QStringLiteral( "kadas/guidegrid/ymin" ), mapLayerEl.attribute( QStringLiteral( "ymin" ) ).toDouble() );
      annoLayer->setCustomProperty( QStringLiteral( "kadas/guidegrid/xmax" ), mapLayerEl.attribute( QStringLiteral( "xmax" ) ).toDouble() );
      annoLayer->setCustomProperty( QStringLiteral( "kadas/guidegrid/ymax" ), mapLayerEl.attribute( QStringLiteral( "ymax" ) ).toDouble() );
      annoLayer->setCustomProperty( QStringLiteral( "kadas/guidegrid/cols" ), mapLayerEl.attribute( QStringLiteral( "cols" ) ).toInt() );
      annoLayer->setCustomProperty( QStringLiteral( "kadas/guidegrid/rows" ), mapLayerEl.attribute( QStringLiteral( "rows" ) ).toInt() );
      annoLayer->setCustomProperty( QStringLiteral( "kadas/guidegrid/colSizeLocked" ), mapLayerEl.attribute( QStringLiteral( "colSizeLocked" ) ) == QLatin1String( "1" ) );
      annoLayer->setCustomProperty( QStringLiteral( "kadas/guidegrid/rowSizeLocked" ), mapLayerEl.attribute( QStringLiteral( "rowSizeLocked" ) ) == QLatin1String( "1" ) );
      annoLayer->setCustomProperty( QStringLiteral( "kadas/guidegrid/fontSize" ), mapLayerEl.attribute( QStringLiteral( "fontSize" ), QStringLiteral( "30" ) ).toInt() );
      annoLayer->setCustomProperty( QStringLiteral( "kadas/guidegrid/color" ), mapLayerEl.attribute( QStringLiteral( "color" ), QStringLiteral( "255,0,0,255" ) ) );
      annoLayer->setCustomProperty( QStringLiteral( "kadas/guidegrid/lineWidth" ), mapLayerEl.attribute( QStringLiteral( "lineWidth" ), QStringLiteral( "1" ) ).toInt() );
      annoLayer->setCustomProperty( QStringLiteral( "kadas/guidegrid/rowChar" ), mapLayerEl.attribute( QStringLiteral( "rowChar" ), QStringLiteral( "A" ) ) );
      annoLayer->setCustomProperty( QStringLiteral( "kadas/guidegrid/colChar" ), mapLayerEl.attribute( QStringLiteral( "colChar" ), QStringLiteral( "1" ) ) );
      annoLayer->setCustomProperty( QStringLiteral( "kadas/guidegrid/labelingPos" ), mapLayerEl.attribute( QStringLiteral( "labelingPos" ), QStringLiteral( "0" ) ).toInt() );
      annoLayer->setCustomProperty( QStringLiteral( "kadas/guidegrid/quadrantLabeling" ), mapLayerEl.attribute( QStringLiteral( "quadrantLabeling" ), QStringLiteral( "0" ) ).toInt() );
    }

    QDomElement newMapLayerEl = doc.createElement( "maplayer" );
    QgsReadWriteContext context;
    annoLayer->writeLayerXml( newMapLayerEl, doc, context );

    // Preserve the original layer id and name so layer-tree-layer,
    // layerorder and similar references continue to resolve (see
    // migrateLegacyMilxLayers).
    const QString originalId = mapLayerEl.firstChildElement( "id" ).text();
    QDomElement existingId = newMapLayerEl.firstChildElement( "id" );
    if ( !existingId.isNull() )
    {
      QDomElement replacementId = doc.createElement( "id" );
      replacementId.appendChild( doc.createTextNode( originalId ) );
      newMapLayerEl.replaceChild( replacementId, existingId );
    }
    QDomElement existingName = newMapLayerEl.firstChildElement( "layername" );
    if ( !existingName.isNull() )
    {
      QDomElement replacementName = doc.createElement( "layername" );
      replacementName.appendChild( doc.createTextNode( layerName ) );
      newMapLayerEl.replaceChild( replacementName, existingName );
    }

    projectLayersEl.replaceChild( newMapLayerEl, mapLayerEl );
  }

  // Clear the now-defunct plugin providerKey on the matching
  // layer-tree-layer entries so QGIS resolves the layers through the
  // projectlayers maplayer blocks (now annotation layers).
  QDomNodeList treeLayers = root.elementsByTagName( QStringLiteral( "layer-tree-layer" ) );
  for ( int i = 0, n = treeLayers.size(); i < n; ++i )
  {
    QDomElement el = treeLayers.at( i ).toElement();
    const QString providerKey = el.attribute( QStringLiteral( "providerKey" ) );
    if ( providerKey == QLatin1String( "bullseye" ) || providerKey == QLatin1String( "guide_grid" ) )
      el.setAttribute( QStringLiteral( "providerKey" ), QString() );
  }

  return true;
}

QDomElement KadasProjectMigration::replaceAnnotationLayer( QDomDocument &doc, QDomElement &root, const QString &layerId )
{
  QDomElement projectLayersEl = root.firstChildElement( "projectlayers" );
  QDomNodeList maplayers = projectLayersEl.elementsByTagName( "maplayer" );
  for ( int i = 0, n = maplayers.size(); i < n; ++i )
  {
    QDomElement mapLayerEl = maplayers.at( i ).toElement();
    QString layerName = mapLayerEl.firstChildElement( "layername" ).text();
    if ( mapLayerEl.firstChildElement( "id" ).text() == layerId )
    {
      QDomElement newMapLayerEl = doc.createElement( "maplayer" );
      newMapLayerEl.setAttribute( "name", "KadasItemLayer" );
      newMapLayerEl.setAttribute( "type", "plugin" );
      newMapLayerEl.setAttribute( "title", layerName );
      newMapLayerEl.appendChild( mapLayerEl.firstChildElement( "srs" ).cloneNode() );
      newMapLayerEl.appendChild( mapLayerEl.firstChildElement( "id" ).cloneNode() );
      newMapLayerEl.appendChild( mapLayerEl.firstChildElement( "layername" ).cloneNode() );

      projectLayersEl.replaceChild( newMapLayerEl, mapLayerEl );
      return newMapLayerEl;
    }
  }
  return QDomElement();
}

QMap<QString, QString> KadasProjectMigration::deserializeLegacyRedliningFlags( const QString &flagsStr )
{
  QMap<QString, QString> flagsMap;
  foreach ( const QString &flag, flagsStr.split( ",", Qt::SkipEmptyParts ) )
  {
    int pos = flag.indexOf( "=" );
    flagsMap.insert( flag.left( pos ), pos >= 0 ? flag.mid( pos + 1 ) : QString() );
  }
  return flagsMap;
}

bool KadasProjectMigration::shouldAttach( const QString &baseDir, const QString &filePath )
{
  QFile file( QDir( baseDir ).absoluteFilePath( filePath ) );
  // Attach files relative to base dir smaller than 10 MB
  return QFileInfo( filePath ).isRelative() && file.exists() && file.size() < 10 * 1024 * 1024;
}


// ------------------------------------------------------------------------
//   Legacy KadasItemLayer → QgsAnnotationLayer XML rewriter
// ------------------------------------------------------------------------
//
// Per-type translators read the v2 (XML-attribute) `<MapItem>` payload
// produced by `KadasMapItem::writeXml` and return a freshly constructed
// `QgsAnnotationItem` of the matching subclass, with geometry transformed
// from the item's CRS to the target layer CRS. v1 (JSON-in-CDATA) payloads
// produced by released kadas (3.x and earlier) are handled by rewriting
// each `<MapItem>` in-place to the v2 attribute layout via
// `convertV1MapItemToV2` before dispatching to the translator.
//
// New item types are added one per slice. Each translator is small and
// mechanical; failure to translate any single MapItem in a layer aborts
// the rewrite for that layer.

#include <qgis/qgis.h>
#include <qgis/qgsannotationlineitem.h>
#include <qgis/qgsannotationmarkeritem.h>
#include <qgis/qgsannotationpictureitem.h>
#include <qgis/qgsannotationpointtextitem.h>
#include <qgis/qgsannotationpolygonitem.h>
#include <qgis/qgscircularstring.h>
#include <qgis/qgscompoundcurve.h>
#include <qgis/qgscoordinatetransform.h>
#include <qgis/qgsfillsymbol.h>
#include <qgis/qgsfillsymbollayer.h>
#include <qgis/qgscallout.h>
#include <qgis/qgslinesymbol.h>
#include <qgis/qgslinesymbollayer.h>
#include <qgis/qgsmarkersymbol.h>
#include <qgis/qgsmarkersymbollayer.h>
#include <qgis/qgsmultilinestring.h>
#include <qgis/qgsmultipolygon.h>
#include <qgis/qgsreadwritecontext.h>
#include <qgis/qgstextformat.h>

#include "kadas/gui/annotationitems/kadasrectangleannotationitem.h"
#include "kadas/gui/annotationitems/kadascircleannotationitem.h"
#include "kadas/gui/annotationitems/kadascoordcrossannotationitem.h"
#include "kadas/gui/annotationitems/kadaspictureannotationcontroller.h"
#include "kadas/gui/annotationitems/kadaspinannotationitem.h"
#include "kadas/gui/annotationitems/kadasgpxrouteannotationitem.h"
#include "kadas/gui/annotationitems/kadasgpxwaypointannotationitem.h"

namespace
{
  // ---------------------------------------------------------------------
  //   v1 (JSON-CDATA) → v2 (typed XML attributes) conversion
  // ---------------------------------------------------------------------
  //
  // Released kadas (3.x and earlier) wrote each `<MapItem>` as a JSON
  // payload inside a CDATA child rather than as typed XML attributes.
  // Phase 6 (slices 103–122) flipped every `KadasMapItem` subclass to
  // the typed-XML format consumed by the v2 translators below. Once the
  // post-load `KadasItemLayerMigration` fallback was deleted in Phase 7,
  // legacy `.qgz` projects on disk had no way back. The helpers below
  // translate the JSON `props` / `state` shape produced by the deleted
  // `KadasMapItem::serialize()` family into v2 attributes in-place, so
  // the existing `translateKadas*Item` chain handles them unchanged.

  // Parse a string of the form "#aarrggbb;width;penStyle" written by
  // legacy `KadasGeometryItem` for outline pens.
  void parseV1Pen( const QString &s, QColor &color, int &width, int &penStyle )
  {
    const QStringList parts = s.split( QChar( ';' ) );
    if ( !parts.isEmpty() && QColor::isValidColorName( parts.at( 0 ) ) )
      color = QColor( parts.at( 0 ) );
    if ( parts.size() > 1 )
      width = parts.at( 1 ).toInt();
    if ( parts.size() > 2 )
      penStyle = parts.at( 2 ).toInt();
  }

  // Parse a string of the form "#aarrggbb;style" written by legacy
  // `KadasGeometryItem` for fill brushes.
  void parseV1Brush( const QString &s, QColor &color, int &brushStyle )
  {
    const QStringList parts = s.split( QChar( ';' ) );
    if ( !parts.isEmpty() && QColor::isValidColorName( parts.at( 0 ) ) )
      color = QColor( parts.at( 0 ) );
    if ( parts.size() > 1 )
      brushStyle = parts.at( 1 ).toInt();
  }

  // Read a [x, y] pair from a QJsonValue holding an array. Returns false
  // if the value is not a 2-element array.
  bool readJsonPoint( const QJsonValue &v, double &x, double &y )
  {
    if ( !v.isArray() )
      return false;
    const QJsonArray a = v.toArray();
    if ( a.size() < 2 )
      return false;
    x = a.at( 0 ).toDouble();
    y = a.at( 1 ).toDouble();
    return true;
  }

  // Format a list of points as the legacy "x,y;x,y;..." string used by
  // the v2 p1/p2/centers/ringpos attributes.
  QString formatPointList( const QList<QgsPointXY> &pts )
  {
    QStringList out;
    out.reserve( pts.size() );
    for ( const QgsPointXY &p : pts )
      out << QStringLiteral( "%1,%2" ).arg( p.x(), 0, 'g', 17 ).arg( p.y(), 0, 'g', 17 );
    return out.join( QChar( ';' ) );
  }

  /**
   * Rewrite \a itemEl (a `<MapItem>` element holding a v1 JSON-CDATA
   * payload) in-place to the v2 attribute layout consumed by the
   * `translateKadas*Item` helpers below. Removes the CDATA child on
   * success and sets `format_version="2"`. Returns false (and leaves the
   * element unchanged) for unrecognised types or malformed JSON.
   */
  bool convertV1MapItemToV2( QDomElement &itemEl )
  {
    // Already v2 — nothing to do.
    if ( itemEl.attribute( QStringLiteral( "format_version" ) ) == QLatin1String( "2" ) )
      return true;

    // Locate the CDATA child carrying the JSON payload.
    QDomNode cdataNode;
    for ( QDomNode n = itemEl.firstChild(); !n.isNull(); n = n.nextSibling() )
    {
      if ( n.isCDATASection() )
      {
        cdataNode = n;
        break;
      }
    }
    if ( cdataNode.isNull() )
      return false;

    const QByteArray cdata = cdataNode.toCDATASection().data().toUtf8();
    QJsonParseError perr;
    const QJsonDocument jd = QJsonDocument::fromJson( cdata, &perr );
    if ( perr.error != QJsonParseError::NoError || !jd.isObject() )
      return false;

    const QJsonObject root = jd.object();
    const QJsonObject props = root.value( QStringLiteral( "props" ) ).toObject();
    const QJsonObject state = root.value( QStringLiteral( "state" ) ).toObject();
    const QString name = itemEl.attribute( QStringLiteral( "name" ) );

    auto setAttr = [&]( const char *k, const QString &v ) { itemEl.setAttribute( QLatin1String( k ), v ); };

    // Common props that map 1:1 onto v2 attributes.
    if ( props.contains( QStringLiteral( "tooltip" ) ) )
      setAttr( "tooltip", props.value( QStringLiteral( "tooltip" ) ).toString() );
    if ( props.contains( QStringLiteral( "zIndex" ) ) )
      setAttr( "z_index", QString::number( props.value( QStringLiteral( "zIndex" ) ).toInt() ) );

    // Fan out the v1 outline/fill props into the v2 geometry-base attrs
    // (used by Line/Polygon/Rectangle/Circle/CircularSector translators).
    auto applyGeometryBase = [&]() {
      QColor outlineColor( Qt::red );
      int outlineWidth = 1;
      int outlinePenStyle = static_cast<int>( Qt::SolidLine );
      QColor fillColor( Qt::transparent );
      int fillBrushStyle = static_cast<int>( Qt::SolidPattern );
      if ( props.contains( QStringLiteral( "outline" ) ) )
        parseV1Pen( props.value( QStringLiteral( "outline" ) ).toString(), outlineColor, outlineWidth, outlinePenStyle );
      if ( props.contains( QStringLiteral( "fill" ) ) )
        parseV1Brush( props.value( QStringLiteral( "fill" ) ).toString(), fillColor, fillBrushStyle );
      setAttr( "outline_color", outlineColor.name( QColor::HexArgb ) );
      setAttr( "outline_width", QString::number( outlineWidth ) );
      setAttr( "outline_style", QString::number( outlinePenStyle ) );
      setAttr( "fill_color", fillColor.name( QColor::HexArgb ) );
      setAttr( "fill_style", QString::number( fillBrushStyle ) );
    };

    auto pointWkt = []( double x, double y ) { return QStringLiteral( "POINT(%1 %2)" ).arg( x, 0, 'g', 17 ).arg( y, 0, 'g', 17 ); };

    if ( name == QLatin1String( "KadasPointItem" ) )
    {
      const QJsonArray points = state.value( QStringLiteral( "points" ) ).toArray();
      if ( points.isEmpty() )
        return false;
      const QJsonArray pt = points.at( 0 ).toArray();
      if ( pt.size() < 2 )
        return false;
      setAttr( "geometry", pointWkt( pt.at( 0 ).toDouble(), pt.at( 1 ).toDouble() ) );

      // Map legacy iconType → Qgis::MarkerShape (verbatim from the
      // deleted KadasPointItem::deserialize switch).
      const int iconType = props.value( QStringLiteral( "iconType" ) ).toInt( 1 );
      Qgis::MarkerShape shape = Qgis::MarkerShape::Circle;
      switch ( iconType )
      {
        case 1:
          shape = Qgis::MarkerShape::Cross;
          break;
        case 2:
          shape = Qgis::MarkerShape::Cross2;
          break;
        case 3:
          shape = Qgis::MarkerShape::Square;
          break;
        case 4:
          shape = Qgis::MarkerShape::Circle;
          break;
        case 5:
          shape = Qgis::MarkerShape::Square;
          break;
        case 6:
          shape = Qgis::MarkerShape::Triangle;
          break;
        case 7:
          shape = Qgis::MarkerShape::Triangle;
          break;
        default:
          break;
      }
      setAttr( "shape", qgsEnumValueToKey( shape ) );
      if ( props.contains( QStringLiteral( "iconSize" ) ) )
        setAttr( "size", QString::number( props.value( QStringLiteral( "iconSize" ) ).toInt() ) );

      // Prefer iconOutline/iconFill, fall back to outline/fill (older
      // payloads stored only the geometry-level keys).
      QColor strokeColor( Qt::red );
      int strokeWidth = 1;
      int penStyle = static_cast<int>( Qt::SolidLine );
      const QString iconOutline = props.value( QStringLiteral( "iconOutline" ) ).toString();
      parseV1Pen( !iconOutline.isEmpty() ? iconOutline : props.value( QStringLiteral( "outline" ) ).toString(), strokeColor, strokeWidth, penStyle );
      setAttr( "stroke_color", strokeColor.name( QColor::HexArgb ) );
      setAttr( "stroke_width", QString::number( strokeWidth ) );

      QColor markerFill( Qt::white );
      int brushStyle = static_cast<int>( Qt::SolidPattern );
      const QString iconFill = props.value( QStringLiteral( "iconFill" ) ).toString();
      parseV1Brush( !iconFill.isEmpty() ? iconFill : props.value( QStringLiteral( "fill" ) ).toString(), markerFill, brushStyle );
      // Legacy ICON_BOX (3) and ICON_TRIANGLE (6) painted lines only —
      // drop the fill for those variants to match the released look.
      if ( iconType == 3 || iconType == 6 )
        markerFill = QColor( Qt::transparent );
      setAttr( "fill_color", markerFill.name( QColor::HexArgb ) );
    }
    else if ( name == QLatin1String( "KadasTextItem" ) )
    {
      double x = 0, y = 0;
      if ( !readJsonPoint( state.value( QStringLiteral( "pos" ) ), x, y ) )
        return false;
      setAttr( "geometry", pointWkt( x, y ) );
      setAttr( "text", props.value( QStringLiteral( "text" ) ).toString() );
      if ( props.contains( QStringLiteral( "fillColor" ) ) )
        setAttr( "color", props.value( QStringLiteral( "fillColor" ) ).toString() );
      if ( props.contains( QStringLiteral( "outlineColor" ) ) )
        setAttr( "outline_color", props.value( QStringLiteral( "outlineColor" ) ).toString() );
      if ( props.contains( QStringLiteral( "font" ) ) )
        setAttr( "font", props.value( QStringLiteral( "font" ) ).toString() );
      setAttr( "angle", QString::number( state.value( QStringLiteral( "angle" ) ).toDouble() ) );
    }
    else if ( name == QLatin1String( "KadasLineItem" ) || name == QLatin1String( "KadasGpxRouteItem" ) )
    {
      // state.points = [ [ [x,y], … ], … ] — list of parts.
      const QJsonArray parts = state.value( QStringLiteral( "points" ) ).toArray();
      if ( parts.isEmpty() )
        return false;
      QStringList partWkts;
      for ( const QJsonValue &partV : parts )
      {
        const QJsonArray pts = partV.toArray();
        if ( pts.size() < 2 )
          continue;
        QStringList coords;
        for ( const QJsonValue &p : pts )
        {
          const QJsonArray xy = p.toArray();
          if ( xy.size() < 2 )
            continue;
          coords << QStringLiteral( "%1 %2" ).arg( xy.at( 0 ).toDouble(), 0, 'g', 17 ).arg( xy.at( 1 ).toDouble(), 0, 'g', 17 );
        }
        if ( coords.size() >= 2 )
          partWkts << QStringLiteral( "(%1)" ).arg( coords.join( QStringLiteral( ", " ) ) );
      }
      if ( partWkts.isEmpty() )
        return false;
      setAttr( "geometry", QStringLiteral( "MULTILINESTRING(%1)" ).arg( partWkts.join( QStringLiteral( ", " ) ) ) );
      applyGeometryBase();
      if ( name == QLatin1String( "KadasGpxRouteItem" ) )
      {
        if ( props.contains( QStringLiteral( "name" ) ) )
          setAttr( "gpx_name", props.value( QStringLiteral( "name" ) ).toString() );
        if ( props.contains( QStringLiteral( "number" ) ) )
          setAttr( "gpx_number", QString::number( props.value( QStringLiteral( "number" ) ).toInt() ) );
      }
    }
    else if ( name == QLatin1String( "KadasPolygonItem" ) )
    {
      // state.points = [ [ [x,y], … ], … ] — list of exterior rings.
      // The legacy item never stored interior rings, so emit one
      // single-ring polygon part per ring.
      const QJsonArray rings = state.value( QStringLiteral( "points" ) ).toArray();
      if ( rings.isEmpty() )
        return false;
      QStringList polyWkts;
      for ( const QJsonValue &ringV : rings )
      {
        const QJsonArray pts = ringV.toArray();
        if ( pts.size() < 3 )
          continue;
        QStringList coords;
        for ( const QJsonValue &p : pts )
        {
          const QJsonArray xy = p.toArray();
          if ( xy.size() < 2 )
            continue;
          coords << QStringLiteral( "%1 %2" ).arg( xy.at( 0 ).toDouble(), 0, 'g', 17 ).arg( xy.at( 1 ).toDouble(), 0, 'g', 17 );
        }
        if ( coords.size() < 3 )
          continue;
        if ( coords.first() != coords.last() )
          coords << coords.first();
        polyWkts << QStringLiteral( "((%1))" ).arg( coords.join( QStringLiteral( ", " ) ) );
      }
      if ( polyWkts.isEmpty() )
        return false;
      setAttr( "geometry", QStringLiteral( "MULTIPOLYGON(%1)" ).arg( polyWkts.join( QStringLiteral( ", " ) ) ) );
      applyGeometryBase();
    }
    else if ( name == QLatin1String( "KadasRectangleItem" ) )
    {
      const QJsonArray p1arr = state.value( QStringLiteral( "p1" ) ).toArray();
      const QJsonArray p2arr = state.value( QStringLiteral( "p2" ) ).toArray();
      if ( p1arr.isEmpty() || p1arr.size() != p2arr.size() )
        return false;
      QList<QgsPointXY> p1List, p2List;
      for ( int i = 0; i < p1arr.size(); ++i )
      {
        const QJsonArray a = p1arr.at( i ).toArray();
        const QJsonArray b = p2arr.at( i ).toArray();
        if ( a.size() < 2 || b.size() < 2 )
          return false;
        p1List << QgsPointXY( a.at( 0 ).toDouble(), a.at( 1 ).toDouble() );
        p2List << QgsPointXY( b.at( 0 ).toDouble(), b.at( 1 ).toDouble() );
      }
      setAttr( "p1", formatPointList( p1List ) );
      setAttr( "p2", formatPointList( p2List ) );
      applyGeometryBase();
    }
    else if ( name == QLatin1String( "KadasCircleItem" ) )
    {
      const QJsonArray ca = state.value( QStringLiteral( "centers" ) ).toArray();
      const QJsonArray ra = state.value( QStringLiteral( "ringpos" ) ).toArray();
      if ( ca.isEmpty() || ca.size() != ra.size() )
        return false;
      QList<QgsPointXY> centers, ringpos;
      for ( int i = 0; i < ca.size(); ++i )
      {
        const QJsonArray a = ca.at( i ).toArray();
        const QJsonArray b = ra.at( i ).toArray();
        if ( a.size() < 2 || b.size() < 2 )
          return false;
        centers << QgsPointXY( a.at( 0 ).toDouble(), a.at( 1 ).toDouble() );
        ringpos << QgsPointXY( b.at( 0 ).toDouble(), b.at( 1 ).toDouble() );
      }
      setAttr( "centers", formatPointList( centers ) );
      setAttr( "ringpos", formatPointList( ringpos ) );
      applyGeometryBase();
    }
    else if ( name == QLatin1String( "KadasPictureItem" ) )
    {
      double x = 0, y = 0;
      if ( !readJsonPoint( state.value( QStringLiteral( "pos" ) ), x, y ) )
        return false;
      setAttr( "pos_x", QString::number( x, 'g', 17 ) );
      setAttr( "pos_y", QString::number( y, 'g', 17 ) );
      setAttr( "offset_x", QString::number( state.value( QStringLiteral( "offsetX" ) ).toDouble() ) );
      setAttr( "offset_y", QString::number( state.value( QStringLiteral( "offsetY" ) ).toDouble() ) );
      const QJsonArray sz = state.value( QStringLiteral( "size" ) ).toArray();
      if ( sz.size() >= 2 )
      {
        setAttr( "size_w", QString::number( sz.at( 0 ).toInt() ) );
        setAttr( "size_h", QString::number( sz.at( 1 ).toInt() ) );
      }
      setAttr( "frame", state.value( QStringLiteral( "frame" ) ).toBool( true ) ? QStringLiteral( "1" ) : QStringLiteral( "0" ) );
      if ( props.contains( QStringLiteral( "filePath" ) ) )
      {
        QString fp = props.value( QStringLiteral( "filePath" ) ).toString();
        // Legacy stored archive attachments as `attachment:<name>`,
        // but QgsProject::resolveAttachmentIdentifier only accepts the
        // canonical `attachment:///<name>` form. Normalize on the fly
        // so the post-load resolver in KadasAnnotationProjectIntegration
        // can locate the file inside the migrated `.qgz`.
        if ( fp.startsWith( QLatin1String( "attachment:" ) ) && !fp.startsWith( QLatin1String( "attachment:///" ) ) )
          fp = QStringLiteral( "attachment:///" ) + fp.mid( 11 );
        setAttr( "file_path", fp );
      }
    }
    else if ( name == QLatin1String( "KadasSymbolItem" ) )
    {
      double x = 0, y = 0;
      if ( !readJsonPoint( state.value( QStringLiteral( "pos" ) ), x, y ) )
        return false;
      setAttr( "pos_x", QString::number( x, 'g', 17 ) );
      setAttr( "pos_y", QString::number( y, 'g', 17 ) );
      if ( props.contains( QStringLiteral( "anchorX" ) ) )
        setAttr( "anchor_x", QString::number( props.value( QStringLiteral( "anchorX" ) ).toDouble() ) );
      if ( props.contains( QStringLiteral( "anchorY" ) ) )
        setAttr( "anchor_y", QString::number( props.value( QStringLiteral( "anchorY" ) ).toDouble() ) );
      const QJsonArray sz = state.value( QStringLiteral( "size" ) ).toArray();
      if ( sz.size() >= 2 )
      {
        setAttr( "size_w", QString::number( sz.at( 0 ).toInt() ) );
        setAttr( "size_h", QString::number( sz.at( 1 ).toInt() ) );
      }
      if ( props.contains( QStringLiteral( "filePath" ) ) )
      {
        QString fp = props.value( QStringLiteral( "filePath" ) ).toString();
        if ( fp.startsWith( QLatin1String( "attachment:" ) ) && !fp.startsWith( QLatin1String( "attachment:///" ) ) )
          fp = QStringLiteral( "attachment:///" ) + fp.mid( 11 );
        setAttr( "file_path", fp );
      }
    }
    else if ( name == QLatin1String( "KadasPinItem" ) )
    {
      double x = 0, y = 0;
      if ( !readJsonPoint( state.value( QStringLiteral( "pos" ) ), x, y ) )
        return false;
      setAttr( "pos_x", QString::number( x, 'g', 17 ) );
      setAttr( "pos_y", QString::number( y, 'g', 17 ) );
      // The v2 KadasPinItem writer clobbered the class-name `name`
      // attribute with the display name (legacy bug; see the
      // translatesPinItem unit test). Route the v1 user name to a
      // distinct `pin_name` attribute so the dispatcher's `name`-based
      // type lookup still works; `translateKadasPinItem` prefers
      // `pin_name` over `name`.
      if ( props.contains( QStringLiteral( "name" ) ) )
        setAttr( "pin_name", props.value( QStringLiteral( "name" ) ).toString() );
      if ( props.contains( QStringLiteral( "remarks" ) ) )
        setAttr( "remarks", props.value( QStringLiteral( "remarks" ) ).toString() );
    }
    else if ( name == QLatin1String( "KadasGpxWaypointItem" ) )
    {
      double x = 0, y = 0;
      if ( !readJsonPoint( state.value( QStringLiteral( "pos" ) ), x, y ) )
      {
        // Older payloads stored a "points" list rather than "pos".
        const QJsonArray points = state.value( QStringLiteral( "points" ) ).toArray();
        if ( points.isEmpty() )
          return false;
        const QJsonArray pt = points.at( 0 ).toArray();
        if ( pt.size() < 2 )
          return false;
        x = pt.at( 0 ).toDouble();
        y = pt.at( 1 ).toDouble();
      }
      setAttr( "geometry", pointWkt( x, y ) );
      setAttr( "shape", qgsEnumValueToKey( Qgis::MarkerShape::Circle ) );
      if ( props.contains( QStringLiteral( "name" ) ) )
        setAttr( "gpx_name", props.value( QStringLiteral( "name" ) ).toString() );
    }
    else if ( name == QLatin1String( "KadasCoordinateCrossItem" ) )
    {
      // Legacy CoordinateCross stored x/y directly on the state. Keep
      // the original element name; the dispatcher routes to a dedicated
      // translator that builds a real KadasCoordCrossAnnotationItem
      // (full-screen cross + km labels) instead of a tiny "+" marker.
      const double x = state.value( QStringLiteral( "x" ) ).toDouble();
      const double y = state.value( QStringLiteral( "y" ) ).toDouble();
      setAttr( "geometry", pointWkt( x, y ) );
    }
    else
    {
      // Unknown legacy type — leave the element untouched and let the
      // dispatcher abort the layer.
      return false;
    }

    itemEl.setAttribute( QStringLiteral( "format_version" ), QStringLiteral( "2" ) );
    itemEl.removeChild( cdataNode );
    return true;
  }

  /**
   * Translate one `<MapItem name="KadasPointItem">` (v2 format) into a
   * fresh `QgsAnnotationMarkerItem`. Returns nullptr for v1 payloads or
   * any unrecoverable parse error.
   */
  QgsAnnotationMarkerItem *translateKadasPointItem( const QDomElement &itemEl, const QgsCoordinateReferenceSystem &itemCrs, const QgsCoordinateReferenceSystem &layerCrs )
  {
    if ( itemEl.attribute( QStringLiteral( "format_version" ), QStringLiteral( "1" ) ) != QLatin1String( "2" ) )
      return nullptr;

    const Qgis::MarkerShape shape = qgsEnumKeyToValue( itemEl.attribute( QStringLiteral( "shape" ), qgsEnumValueToKey( Qgis::MarkerShape::Circle ) ), Qgis::MarkerShape::Circle );
    const int size = itemEl.attribute( QStringLiteral( "size" ), QStringLiteral( "4" ) ).toInt();
    const QColor strokeColor( itemEl.attribute( QStringLiteral( "stroke_color" ), QColor( Qt::red ).name() ) );
    const int strokeWidth = itemEl.attribute( QStringLiteral( "stroke_width" ), QStringLiteral( "1" ) ).toInt();
    const QColor fillColor( itemEl.attribute( QStringLiteral( "fill_color" ), QColor( Qt::white ).name() ) );

    const QgsGeometry geom = QgsGeometry::fromWkt( itemEl.attribute( QStringLiteral( "geometry" ) ) );
    if ( geom.isNull() || geom.type() != Qgis::GeometryType::Point )
      return nullptr;
    QgsPointXY pt = geom.asPoint();
    if ( itemCrs.isValid() && layerCrs.isValid() && itemCrs != layerCrs )
    {
      try
      {
        QgsCoordinateTransform ct( itemCrs, layerCrs, QgsProject::instance() );
        pt = ct.transform( pt );
      }
      catch ( QgsCsException & )
      {
        return nullptr;
      }
    }

    auto *symbolLayer = new QgsSimpleMarkerSymbolLayer();
    symbolLayer->setSizeUnit( Qgis::RenderUnit::Points );
    symbolLayer->setShape( shape );
    symbolLayer->setSize( size );
    symbolLayer->setStrokeWidth( strokeWidth );
    symbolLayer->setStrokeWidthUnit( Qgis::RenderUnit::Points );
    // Order matters: QgsSimpleMarkerSymbolLayer::setColor() delegates to
    // setStrokeColor() for shapes that aren't filled (Cross, Cross2,
    // ...), so call setColor() BEFORE setStrokeColor() to ensure the
    // user's stroke colour wins for line-only shapes.
    symbolLayer->setColor( fillColor );
    symbolLayer->setStrokeColor( strokeColor );

    auto *anno = new QgsAnnotationMarkerItem( QgsPoint( pt ) );
    anno->setSymbol( new QgsMarkerSymbol( QgsSymbolLayerList { symbolLayer } ) );
    return anno;
  }

  /**
   * Translate one `<MapItem name="KadasCoordinateCrossItem">` (v1
   * legacy, rewritten in-place to v2 with just a `geometry` attribute)
   * into a `KadasCoordCrossAnnotationItem`. The legacy item drew a
   * full-screen perpendicular cross with km labels — replicated here
   * by the dedicated annotation type.
   */
  KadasCoordCrossAnnotationItem *translateKadasCoordinateCrossItem( const QDomElement &itemEl, const QgsCoordinateReferenceSystem &itemCrs, const QgsCoordinateReferenceSystem &layerCrs )
  {
    const QgsGeometry geom = QgsGeometry::fromWkt( itemEl.attribute( QStringLiteral( "geometry" ) ) );
    if ( geom.isNull() || geom.type() != Qgis::GeometryType::Point )
      return nullptr;
    QgsPointXY pt = geom.asPoint();
    if ( itemCrs.isValid() && layerCrs.isValid() && itemCrs != layerCrs )
    {
      try
      {
        QgsCoordinateTransform ct( itemCrs, layerCrs, QgsProject::instance() );
        pt = ct.transform( pt );
      }
      catch ( QgsCsException & )
      {
        return nullptr;
      }
    }
    return new KadasCoordCrossAnnotationItem( QgsPoint( pt ) );
  }

  /**
   * Translate one `<MapItem name="KadasTextItem">` (v2 format) into a
   * fresh `QgsAnnotationPointTextItem`. Returns nullptr for v1 payloads
   * or any unrecoverable parse error.
   */
  QgsAnnotationPointTextItem *translateKadasTextItem( const QDomElement &itemEl, const QgsCoordinateReferenceSystem &itemCrs, const QgsCoordinateReferenceSystem &layerCrs )
  {
    if ( itemEl.attribute( QStringLiteral( "format_version" ), QStringLiteral( "1" ) ) != QLatin1String( "2" ) )
      return nullptr;

    const QString text = itemEl.attribute( QStringLiteral( "text" ) );
    const QColor color( itemEl.attribute( QStringLiteral( "color" ) ) );
    const QColor outlineColor( itemEl.attribute( QStringLiteral( "outline_color" ) ) );
    const QString fontStr = itemEl.attribute( QStringLiteral( "font" ) );
    const double angle = itemEl.attribute( QStringLiteral( "angle" ), QStringLiteral( "0" ) ).toDouble();

    const QgsGeometry geom = QgsGeometry::fromWkt( itemEl.attribute( QStringLiteral( "geometry" ) ) );
    if ( geom.isNull() || geom.type() != Qgis::GeometryType::Point )
      return nullptr;
    QgsPointXY pt = geom.asPoint();
    if ( itemCrs.isValid() && layerCrs.isValid() && itemCrs != layerCrs )
    {
      try
      {
        QgsCoordinateTransform ct( itemCrs, layerCrs, QgsProject::instance() );
        pt = ct.transform( pt );
      }
      catch ( QgsCsException & )
      {
        return nullptr;
      }
    }

    QFont font;
    if ( !fontStr.isEmpty() )
      font.fromString( fontStr );

    QgsTextFormat fmt;
    fmt.setFont( font );
    if ( font.pointSize() > 0 )
      fmt.setSize( font.pointSize() );
    fmt.setSizeUnit( Qgis::RenderUnit::Points );
    if ( color.isValid() )
      fmt.setColor( color );
    if ( outlineColor.isValid() )
    {
      QgsTextBufferSettings buffer;
      buffer.setEnabled( true );
      buffer.setColor( outlineColor );
      buffer.setOpacity( outlineColor.alpha() / 255.0 );
      buffer.setSize( 1 );
      fmt.setBuffer( buffer );
    }

    auto *anno = new QgsAnnotationPointTextItem( text, QgsPoint( pt ) );
    anno->setFormat( fmt );
    anno->setAngle( angle );
    return anno;
  }

  /**
   * Read the common geometry-base attributes (outline_color/width/style,
   * fill_color/style) written by `KadasGeometryItem::writeGeometryBaseAttributes`.
   * Returns true if at least the outline color parsed successfully.
   */
  void parseGeometryBaseAttributes( const QDomElement &itemEl, QColor &outlineColor, double &outlineWidth, Qt::PenStyle &outlineStyle, QColor &fillColor, Qt::BrushStyle &fillStyle )
  {
    outlineColor = QColor( itemEl.attribute( QStringLiteral( "outline_color" ), QColor( Qt::red ).name() ) );
    outlineWidth = itemEl.attribute( QStringLiteral( "outline_width" ), QStringLiteral( "1" ) ).toDouble();
    outlineStyle = static_cast<Qt::PenStyle>( itemEl.attribute( QStringLiteral( "outline_style" ), QString::number( static_cast<int>( Qt::SolidLine ) ) ).toInt() );
    fillColor = QColor( itemEl.attribute( QStringLiteral( "fill_color" ), QColor( Qt::transparent ).name( QColor::HexArgb ) ) );
    fillStyle = static_cast<Qt::BrushStyle>( itemEl.attribute( QStringLiteral( "fill_style" ), QString::number( static_cast<int>( Qt::SolidPattern ) ) ).toInt() );
  }

  /**
   * Translate one `<MapItem name="KadasLineItem">` (v2 format) into one
   * `QgsAnnotationLineItem` per part of the underlying MultiLineString.
   */
  QList<QgsAnnotationItem *> translateKadasLineItem( const QDomElement &itemEl, const QgsCoordinateReferenceSystem &itemCrs, const QgsCoordinateReferenceSystem &layerCrs )
  {
    QList<QgsAnnotationItem *> out;
    if ( itemEl.attribute( QStringLiteral( "format_version" ), QStringLiteral( "1" ) ) != QLatin1String( "2" ) )
      return out;

    QColor outlineColor;
    double outlineWidth = 1.0;
    Qt::PenStyle outlineStyle = Qt::SolidLine;
    QColor fillColor;
    Qt::BrushStyle fillStyle = Qt::SolidPattern;
    parseGeometryBaseAttributes( itemEl, outlineColor, outlineWidth, outlineStyle, fillColor, fillStyle );

    QgsGeometry geom = QgsGeometry::fromWkt( itemEl.attribute( QStringLiteral( "geometry" ) ) );
    if ( geom.isNull() )
      return out;
    if ( itemCrs.isValid() && layerCrs.isValid() && itemCrs != layerCrs )
    {
      try
      {
        QgsCoordinateTransform ct( itemCrs, layerCrs, QgsProject::instance() );
        geom.transform( ct );
      }
      catch ( QgsCsException & )
      {
        return out;
      }
    }

    const QgsAbstractGeometry *ag = geom.constGet();
    const QgsMultiLineString *mls = dynamic_cast<const QgsMultiLineString *>( ag );
    auto makeSymbol = [&]() {
      auto *layer = new QgsSimpleLineSymbolLayer( outlineColor, outlineWidth, outlineStyle );
      layer->setWidthUnit( Qgis::RenderUnit::Pixels );
      return new QgsLineSymbol( QgsSymbolLayerList { layer } );
    };
    auto addPart = [&]( const QgsLineString *ls ) {
      auto *anno = new QgsAnnotationLineItem( ls->clone() );
      anno->setSymbol( makeSymbol() );
      out.append( anno );
    };
    if ( mls )
    {
      for ( int i = 0, n = mls->numGeometries(); i < n; ++i )
        addPart( static_cast<const QgsLineString *>( mls->geometryN( i ) ) );
    }
    else if ( const auto *ls = dynamic_cast<const QgsLineString *>( ag ) )
    {
      addPart( ls );
    }
    return out;
  }

  /**
   * Translate one `<MapItem name="KadasPolygonItem">` (v2 format) into one
   * `QgsAnnotationPolygonItem` per part of the underlying MultiPolygon.
   */
  QList<QgsAnnotationItem *> translateKadasPolygonItem( const QDomElement &itemEl, const QgsCoordinateReferenceSystem &itemCrs, const QgsCoordinateReferenceSystem &layerCrs )
  {
    QList<QgsAnnotationItem *> out;
    if ( itemEl.attribute( QStringLiteral( "format_version" ), QStringLiteral( "1" ) ) != QLatin1String( "2" ) )
      return out;

    QColor outlineColor;
    double outlineWidth = 1.0;
    Qt::PenStyle outlineStyle = Qt::SolidLine;
    QColor fillColor;
    Qt::BrushStyle fillStyle = Qt::SolidPattern;
    parseGeometryBaseAttributes( itemEl, outlineColor, outlineWidth, outlineStyle, fillColor, fillStyle );

    QgsGeometry geom = QgsGeometry::fromWkt( itemEl.attribute( QStringLiteral( "geometry" ) ) );
    if ( geom.isNull() )
      return out;
    if ( itemCrs.isValid() && layerCrs.isValid() && itemCrs != layerCrs )
    {
      try
      {
        QgsCoordinateTransform ct( itemCrs, layerCrs, QgsProject::instance() );
        geom.transform( ct );
      }
      catch ( QgsCsException & )
      {
        return out;
      }
    }

    auto makeSymbol = [&]() {
      auto *layer = new QgsSimpleFillSymbolLayer( fillColor, fillStyle, outlineColor, outlineStyle, outlineWidth );
      layer->setStrokeWidthUnit( Qgis::RenderUnit::Pixels );
      return new QgsFillSymbol( QgsSymbolLayerList { layer } );
    };
    auto addPart = [&]( const QgsAbstractGeometry *part ) {
      auto *poly = static_cast<QgsCurvePolygon *>( part->clone() );
      auto *anno = new QgsAnnotationPolygonItem( poly );
      anno->setSymbol( makeSymbol() );
      out.append( anno );
    };
    const QgsAbstractGeometry *ag = geom.constGet();
    if ( const QgsMultiPolygon *mp = dynamic_cast<const QgsMultiPolygon *>( ag ) )
    {
      for ( int i = 0, n = mp->numGeometries(); i < n; ++i )
        addPart( mp->geometryN( i ) );
    }
    else if ( const auto *cp = dynamic_cast<const QgsCurvePolygon *>( ag ) )
    {
      addPart( cp );
    }
    return out;
  }

  /**
   * Translate one `<MapItem name="KadasRectangleItem">` (v2 format) into
   * one `KadasRectangleAnnotationItem` per (p1, p2) pair. The legacy item
   * stores axis-aligned diagonals; rotation is always zero.
   */
  QList<QgsAnnotationItem *> translateKadasRectangleItem( const QDomElement &itemEl, const QgsCoordinateReferenceSystem &itemCrs, const QgsCoordinateReferenceSystem &layerCrs )
  {
    QList<QgsAnnotationItem *> out;
    if ( itemEl.attribute( QStringLiteral( "format_version" ), QStringLiteral( "1" ) ) != QLatin1String( "2" ) )
      return out;

    QColor outlineColor;
    double outlineWidth = 1.0;
    Qt::PenStyle outlineStyle = Qt::SolidLine;
    QColor fillColor;
    Qt::BrushStyle fillStyle = Qt::SolidPattern;
    parseGeometryBaseAttributes( itemEl, outlineColor, outlineWidth, outlineStyle, fillColor, fillStyle );

    auto parsePoints = []( const QString &s ) {
      QList<QgsPointXY> pts;
      if ( s.isEmpty() )
        return pts;
      const QStringList pairs = s.split( QChar( ';' ), Qt::SkipEmptyParts );
      for ( const QString &pair : pairs )
      {
        const QStringList xy = pair.split( QChar( ',' ) );
        if ( xy.size() == 2 )
          pts.append( QgsPointXY( xy[0].toDouble(), xy[1].toDouble() ) );
      }
      return pts;
    };
    const QList<QgsPointXY> p1List = parsePoints( itemEl.attribute( QStringLiteral( "p1" ) ) );
    const QList<QgsPointXY> p2List = parsePoints( itemEl.attribute( QStringLiteral( "p2" ) ) );
    if ( p1List.isEmpty() || p1List.size() != p2List.size() )
      return out;

    const bool needTransform = itemCrs.isValid() && layerCrs.isValid() && itemCrs != layerCrs;
    QgsCoordinateTransform ct;
    if ( needTransform )
    {
      try
      {
        ct = QgsCoordinateTransform( itemCrs, layerCrs, QgsProject::instance() );
      }
      catch ( QgsCsException & )
      {
        return out;
      }
    }

    auto makeSymbol = [&]() {
      auto *layer = new QgsSimpleFillSymbolLayer( fillColor, fillStyle, outlineColor, outlineStyle, outlineWidth );
      layer->setStrokeWidthUnit( Qgis::RenderUnit::Pixels );
      return new QgsFillSymbol( QgsSymbolLayerList { layer } );
    };

    for ( int i = 0; i < p1List.size(); ++i )
    {
      const QgsPointXY p1 = p1List[i];
      const QgsPointXY p2 = p2List[i];
      // Layout in the item CRS: the legacy item rendered the box axis-aligned
      // in its own CRS. Keep size in the item CRS and let the annotation item
      // transform the corners per-vertex (drawCrs mechanism), so the shape
      // stays a true rectangle when displayed in the original CRS.
      const QgsPointXY center( 0.5 * ( p1.x() + p2.x() ), 0.5 * ( p1.y() + p2.y() ) );
      const QSizeF size( std::abs( p2.x() - p1.x() ), std::abs( p2.y() - p1.y() ) );
      auto *anno = new KadasRectangleAnnotationItem();
      if ( needTransform )
      {
        QgsPointXY centerLayer;
        try
        {
          centerLayer = ct.transform( center );
        }
        catch ( QgsCsException & )
        {
          delete anno;
          qDeleteAll( out );
          out.clear();
          return out;
        }
        anno->setBox( centerLayer, size, 0.0, itemCrs, layerCrs );
      }
      else
      {
        anno->setBox( center, size, 0.0 );
      }
      anno->setSymbol( makeSymbol() );
      out.append( anno );
    }
    return out;
  }

  /**
   * Translate one `<MapItem name="KadasCircleItem">` (v2 format) into one
   * `KadasCircleAnnotationItem` per (center, ring-point) pair. The legacy
   * `geodesic` flag is dropped — annotations are always rendered in CRS
   * units.
   */
  QList<QgsAnnotationItem *> translateKadasCircleItem( const QDomElement &itemEl, const QgsCoordinateReferenceSystem &itemCrs, const QgsCoordinateReferenceSystem &layerCrs )
  {
    QList<QgsAnnotationItem *> out;
    if ( itemEl.attribute( QStringLiteral( "format_version" ), QStringLiteral( "1" ) ) != QLatin1String( "2" ) )
      return out;

    QColor outlineColor;
    double outlineWidth = 1.0;
    Qt::PenStyle outlineStyle = Qt::SolidLine;
    QColor fillColor;
    Qt::BrushStyle fillStyle = Qt::SolidPattern;
    parseGeometryBaseAttributes( itemEl, outlineColor, outlineWidth, outlineStyle, fillColor, fillStyle );

    auto parsePoints = []( const QString &s ) {
      QList<QgsPointXY> pts;
      if ( s.isEmpty() )
        return pts;
      const QStringList pairs = s.split( QChar( ';' ), Qt::SkipEmptyParts );
      for ( const QString &pair : pairs )
      {
        const QStringList xy = pair.split( QChar( ',' ) );
        if ( xy.size() == 2 )
          pts.append( QgsPointXY( xy[0].toDouble(), xy[1].toDouble() ) );
      }
      return pts;
    };
    const QList<QgsPointXY> centers = parsePoints( itemEl.attribute( QStringLiteral( "centers" ) ) );
    const QList<QgsPointXY> ringpos = parsePoints( itemEl.attribute( QStringLiteral( "ringpos" ) ) );
    if ( centers.isEmpty() || centers.size() != ringpos.size() )
      return out;

    const bool needTransform = itemCrs.isValid() && layerCrs.isValid() && itemCrs != layerCrs;
    QgsCoordinateTransform ct;
    if ( needTransform )
    {
      try
      {
        ct = QgsCoordinateTransform( itemCrs, layerCrs, QgsProject::instance() );
      }
      catch ( QgsCsException & )
      {
        return out;
      }
    }

    auto makeSymbol = [&]() {
      auto *layer = new QgsSimpleFillSymbolLayer( fillColor, fillStyle, outlineColor, outlineStyle, outlineWidth );
      layer->setStrokeWidthUnit( Qgis::RenderUnit::Pixels );
      return new QgsFillSymbol( QgsSymbolLayerList { layer } );
    };

    for ( int i = 0; i < centers.size(); ++i )
    {
      QgsPointXY c = centers[i];
      QgsPointXY r = ringpos[i];
      if ( needTransform )
      {
        try
        {
          c = ct.transform( c );
          r = ct.transform( r );
        }
        catch ( QgsCsException & )
        {
          qDeleteAll( out );
          out.clear();
          return out;
        }
      }
      auto *anno = new KadasCircleAnnotationItem( c, r );
      anno->setSymbol( makeSymbol() );
      out.append( anno );
    }
    return out;
  }

  /**
   * Translate one `<MapItem name="KadasPictureItem">` (v2 format) into a
   * single `QgsAnnotationPictureItem` (type id `picture`). Carries the
   * geographic anchor (\c pos_x/pos_y), pixel offset from anchor
   * (\c offset_x/offset_y), pixel size (\c size_w/size_h), the frame
   * toggle, and the file path. Installs the standard balloon callout so
   * the picture looks the same as a freshly drawn one.
   */
  QgsAnnotationPictureItem *translateKadasPictureItem( const QDomElement &itemEl, const QgsCoordinateReferenceSystem &itemCrs, const QgsCoordinateReferenceSystem &layerCrs )
  {
    if ( itemEl.attribute( QStringLiteral( "format_version" ), QStringLiteral( "1" ) ) != QLatin1String( "2" ) )
      return nullptr;

    const double posX = itemEl.attribute( QStringLiteral( "pos_x" ), QStringLiteral( "0" ) ).toDouble();
    const double posY = itemEl.attribute( QStringLiteral( "pos_y" ), QStringLiteral( "0" ) ).toDouble();
    QgsPointXY anchor( posX, posY );
    if ( itemCrs.isValid() && layerCrs.isValid() && itemCrs != layerCrs )
    {
      try
      {
        QgsCoordinateTransform ct( itemCrs, layerCrs, QgsProject::instance() );
        anchor = ct.transform( anchor );
      }
      catch ( QgsCsException & )
      {
        return nullptr;
      }
    }

    const double offsetX = itemEl.attribute( QStringLiteral( "offset_x" ), QStringLiteral( "0" ) ).toDouble();
    const double offsetY = itemEl.attribute( QStringLiteral( "offset_y" ), QStringLiteral( "50" ) ).toDouble();
    const int w = itemEl.attribute( QStringLiteral( "size_w" ), QStringLiteral( "200" ) ).toInt();
    const int h = itemEl.attribute( QStringLiteral( "size_h" ), QStringLiteral( "150" ) ).toInt();
    const bool frame = itemEl.attribute( QStringLiteral( "frame" ), QStringLiteral( "1" ) ) == QLatin1String( "1" );
    const QString filePath = itemEl.attribute( QStringLiteral( "file_path" ) );

    // Legacy attachments use opaque hash names without standard image
    // extensions (e.g. "foo.fEdEXR"), so formatFromPath() falls back to
    // Unknown which the renderer skips. Force Raster for non-SVG paths
    // so the picture renders; SVGs are still detected by extension.
    const Qgis::PictureFormat fmt = filePath.endsWith( QLatin1String( ".svg" ), Qt::CaseInsensitive ) ? Qgis::PictureFormat::SVG : Qgis::PictureFormat::Raster;
    auto *pic = new QgsAnnotationPictureItem( fmt, filePath, QgsRectangle( anchor.x(), anchor.y(), anchor.x(), anchor.y() ) );
    pic->setPlacementMode( Qgis::AnnotationPlacementMode::FixedSize );
    pic->setFixedSize( QSizeF( w > 0 ? w : 200, h > 0 ? h : 150 ) );
    pic->setFixedSizeUnit( Qgis::RenderUnit::Pixels );
    // Legacy KadasPictureItem: `frame` toggled the *bubble callout*
    // (balloon around the image) — not the QGIS "frame border". Map
    // it onto callout visibility: install the canonical balloon when
    // frame=true, install a hidden balloon (transparent fill/stroke,
    // zero wedge) when frame=false. The annotation's own setFrameEnabled
    // (border) is left at its default.
    KadasPictureAnnotationController::ensureBalloon( pic );
    if ( !frame )
    {
      if ( auto *balloon = dynamic_cast<QgsBalloonCallout *>( pic->callout() ) )
      {
        auto *sl = new QgsSimpleFillSymbolLayer( QColor( 0, 0, 0, 0 ), Qt::SolidPattern, QColor( 0, 0, 0, 0 ), Qt::SolidLine, 0.0 );
        sl->setStrokeWidthUnit( Qgis::RenderUnit::Pixels );
        balloon->setFillSymbol( new QgsFillSymbol( QgsSymbolLayerList() << sl ) );
        balloon->setWedgeWidth( 0 );
        balloon->setWedgeWidthUnit( Qgis::RenderUnit::Pixels );
      }
    }
    // Override the centered default that ensureBalloon installs with the
    // user's saved offset. Coordinate-system conversion:
    //   - Legacy KadasPictureItem stored the offset of the picture's
    //     CENTER from the anchor, in map-Y-up pixel coords.
    //   - QGIS QgsAnnotationRectItem (FixedSize + callout anchor) uses
    //     the offset of the picture's TOP-LEFT from the anchor, in
    //     screen-Y-down pixel coords.
    //   Convert: new_x = legacy_x - w/2;  new_y = -legacy_y - h/2.
    const double effW = w > 0 ? w : 200;
    const double effH = h > 0 ? h : 150;
    const double newOffX = offsetX - effW / 2.0;
    const double newOffY = -offsetY - effH / 2.0;
    pic->setOffsetFromCallout( QSizeF( newOffX, newOffY ) );
    pic->setOffsetFromCalloutUnit( Qgis::RenderUnit::Pixels );
    return pic;
  }

  /**
   * Translate one `<MapItem name="KadasSymbolItem">` (v2 format) into a
   * `QgsAnnotationPictureItem`. Mirrors the runtime fallback in
   * `KadasSymbolItem::annotationItem`: places a fixed-size picture at
   * the geographic anchor, with a balloon callout installed via
   * `KadasPictureAnnotationController::ensureBalloon`. Honours the
   * fractional anchor (anchor_x/anchor_y) by translating it into a
   * pixel `offsetFromCallout`.
   */
  QgsAnnotationPictureItem *translateKadasSymbolItem( const QDomElement &itemEl, const QgsCoordinateReferenceSystem &itemCrs, const QgsCoordinateReferenceSystem &layerCrs )
  {
    if ( itemEl.attribute( QStringLiteral( "format_version" ), QStringLiteral( "1" ) ) != QLatin1String( "2" ) )
      return nullptr;

    const double posX = itemEl.attribute( QStringLiteral( "pos_x" ), QStringLiteral( "0" ) ).toDouble();
    const double posY = itemEl.attribute( QStringLiteral( "pos_y" ), QStringLiteral( "0" ) ).toDouble();
    QgsPointXY anchor( posX, posY );
    if ( itemCrs.isValid() && layerCrs.isValid() && itemCrs != layerCrs )
    {
      try
      {
        QgsCoordinateTransform ct( itemCrs, layerCrs, QgsProject::instance() );
        anchor = ct.transform( anchor );
      }
      catch ( QgsCsException & )
      {
        return nullptr;
      }
    }

    const double ax = itemEl.attribute( QStringLiteral( "anchor_x" ), QStringLiteral( "0.5" ) ).toDouble();
    const double ay = itemEl.attribute( QStringLiteral( "anchor_y" ), QStringLiteral( "0.5" ) ).toDouble();
    const int w = itemEl.attribute( QStringLiteral( "size_w" ), QStringLiteral( "32" ) ).toInt();
    const int h = itemEl.attribute( QStringLiteral( "size_h" ), QStringLiteral( "32" ) ).toInt();
    const QString filePath = itemEl.attribute( QStringLiteral( "file_path" ) );
    const double effW = w > 0 ? w : 32;
    const double effH = h > 0 ? h : 32;

    const Qgis::PictureFormat fmt = filePath.endsWith( QLatin1String( ".svg" ), Qt::CaseInsensitive ) ? Qgis::PictureFormat::SVG : Qgis::PictureFormat::Raster;
    auto *pic = new QgsAnnotationPictureItem( fmt, filePath, QgsRectangle( anchor.x(), anchor.y(), anchor.x(), anchor.y() ) );
    pic->setPlacementMode( Qgis::AnnotationPlacementMode::FixedSize );
    pic->setFixedSize( QSizeF( effW, effH ) );
    pic->setFixedSizeUnit( Qgis::RenderUnit::Pixels );
    KadasPictureAnnotationController::ensureBalloon( pic );
    // Place the image so its fractional anchor (ax, ay) lands on the
    // geographic anchor: the top-left of the frame is offset by
    // (-ax * w, -ay * h) pixels from the callout anchor.
    pic->setOffsetFromCallout( QSizeF( -ax * effW, -ay * effH ) );
    pic->setOffsetFromCalloutUnit( Qgis::RenderUnit::Pixels );
    return pic;
  }

  /**
   * Translate one `<MapItem name="KadasPinItem">` (v2 format) into a
   * `KadasPinAnnotationItem`. Carries name/remarks; size/anchor are
   * dropped (pins have a canonical icon).
   */
  KadasPinAnnotationItem *translateKadasPinItem( const QDomElement &itemEl, const QgsCoordinateReferenceSystem &itemCrs, const QgsCoordinateReferenceSystem &layerCrs )
  {
    if ( itemEl.attribute( QStringLiteral( "format_version" ), QStringLiteral( "1" ) ) != QLatin1String( "2" ) )
      return nullptr;

    const double posX = itemEl.attribute( QStringLiteral( "pos_x" ), QStringLiteral( "0" ) ).toDouble();
    const double posY = itemEl.attribute( QStringLiteral( "pos_y" ), QStringLiteral( "0" ) ).toDouble();
    QgsPointXY p( posX, posY );
    if ( itemCrs.isValid() && layerCrs.isValid() && itemCrs != layerCrs )
    {
      try
      {
        QgsCoordinateTransform ct( itemCrs, layerCrs, QgsProject::instance() );
        p = ct.transform( p );
      }
      catch ( QgsCsException & )
      {
        return nullptr;
      }
    }

    auto *anno = new KadasPinAnnotationItem( QgsPoint( p ) );
    // `pin_name` is the v1-migration-specific attribute; falls back to
    // `name` for projects saved by the v2 writer (which clobbered the
    // class-name `name` with the display name — see slice notes in
    // testkadasprojectmigration.cpp).
    const QString pinName = itemEl.hasAttribute( QStringLiteral( "pin_name" ) ) ? itemEl.attribute( QStringLiteral( "pin_name" ) ) : itemEl.attribute( QStringLiteral( "name" ) );
    anno->setName( pinName );
    anno->setRemarks( itemEl.attribute( QStringLiteral( "remarks" ) ) );
    return anno;
  }

  /**
   * Translate one `<MapItem name="KadasCircularSectorItem">` (v2 format)
   * into one `QgsAnnotationPolygonItem` per (center, radius, startAngle,
   * stopAngle) tuple. The sector outline is built from a `QgsCircularString`
   * arc plus two radii (or a full circle when the angle range covers 2π),
   * matching the legacy `recomputeDerived` formula.
   */
  QList<QgsAnnotationItem *> translateKadasCircularSectorItem( const QDomElement &itemEl, const QgsCoordinateReferenceSystem &itemCrs, const QgsCoordinateReferenceSystem &layerCrs )
  {
    QList<QgsAnnotationItem *> out;
    if ( itemEl.attribute( QStringLiteral( "format_version" ), QStringLiteral( "1" ) ) != QLatin1String( "2" ) )
      return out;

    QColor outlineColor;
    double outlineWidth = 1.0;
    Qt::PenStyle outlineStyle = Qt::SolidLine;
    QColor fillColor;
    Qt::BrushStyle fillStyle = Qt::SolidPattern;
    parseGeometryBaseAttributes( itemEl, outlineColor, outlineWidth, outlineStyle, fillColor, fillStyle );

    auto parsePoints = []( const QString &s ) {
      QList<QgsPointXY> pts;
      if ( s.isEmpty() )
        return pts;
      for ( const QString &pair : s.split( QChar( ';' ), Qt::SkipEmptyParts ) )
      {
        const QStringList xy = pair.split( QChar( ',' ) );
        if ( xy.size() == 2 )
          pts.append( QgsPointXY( xy[0].toDouble(), xy[1].toDouble() ) );
      }
      return pts;
    };
    auto parseDoubles = []( const QString &s ) {
      QList<double> out;
      if ( s.isEmpty() )
        return out;
      for ( const QString &v : s.split( QChar( ';' ), Qt::SkipEmptyParts ) )
        out.append( v.toDouble() );
      return out;
    };
    const QList<QgsPointXY> centers = parsePoints( itemEl.attribute( QStringLiteral( "centers" ) ) );
    const QList<double> radii = parseDoubles( itemEl.attribute( QStringLiteral( "radii" ) ) );
    const QList<double> startAngles = parseDoubles( itemEl.attribute( QStringLiteral( "start_angles" ) ) );
    const QList<double> stopAngles = parseDoubles( itemEl.attribute( QStringLiteral( "stop_angles" ) ) );
    if ( centers.isEmpty() || centers.size() != radii.size() || centers.size() != startAngles.size() || centers.size() != stopAngles.size() )
      return out;

    const bool needTransform = itemCrs.isValid() && layerCrs.isValid() && itemCrs != layerCrs;
    QgsCoordinateTransform ct;
    if ( needTransform )
    {
      try
      {
        ct = QgsCoordinateTransform( itemCrs, layerCrs, QgsProject::instance() );
      }
      catch ( QgsCsException & )
      {
        return out;
      }
    }

    auto makeSymbol = [&]() {
      auto *layer = new QgsSimpleFillSymbolLayer( fillColor, fillStyle, outlineColor, outlineStyle, outlineWidth );
      layer->setStrokeWidthUnit( Qgis::RenderUnit::Pixels );
      return new QgsFillSymbol( QgsSymbolLayerList { layer } );
    };

    for ( int i = 0; i < centers.size(); ++i )
    {
      QgsPointXY center = centers[i];
      const double radius = radii[i];
      const double startAngle = startAngles[i];
      const double stopAngle = stopAngles[i];
      if ( needTransform )
      {
        try
        {
          center = ct.transform( center );
        }
        catch ( QgsCsException & )
        {
          qDeleteAll( out );
          out.clear();
          return out;
        }
      }
      auto *exterior = new QgsCompoundCurve();
      if ( stopAngle - startAngle < 2 * M_PI - std::numeric_limits<float>::epsilon() )
      {
        const double alphaMid = 0.5 * ( startAngle + 2 * M_PI + stopAngle );
        QgsPoint pStart( center.x() + radius * std::cos( startAngle ), center.y() + radius * std::sin( startAngle ) );
        QgsPoint pMid( center.x() + radius * std::cos( alphaMid ), center.y() + radius * std::sin( alphaMid ) );
        QgsPoint pEnd( center.x() + radius * std::cos( stopAngle ), center.y() + radius * std::sin( stopAngle ) );
        exterior->addCurve( new QgsCircularString( pStart, pMid, pEnd ) );
        exterior->addCurve( new QgsLineString( QgsPointSequence() << pEnd << QgsPoint( center ) << pStart ) );
      }
      else
      {
        auto *arc = new QgsCircularString();
        arc->setPoints(
          QgsPointSequence()
          << QgsPoint( center.x(), center.y() + radius )
          << QgsPoint( center.x() + radius, center.y() )
          << QgsPoint( center.x(), center.y() - radius )
          << QgsPoint( center.x() - radius, center.y() )
          << QgsPoint( center.x(), center.y() + radius )
        );
        exterior->addCurve( arc );
      }
      auto *poly = new QgsCurvePolygon();
      poly->setExteriorRing( exterior );
      auto *anno = new QgsAnnotationPolygonItem( poly );
      anno->setSymbol( makeSymbol() );
      out.append( anno );
    }
    return out;
  }

  /**
   * Translate one `<MapItem name="KadasGpxRouteItem">` (v2 format) into
   * one `KadasGpxRouteAnnotationItem` per part of the underlying
   * MultiLineString. Carries gpx_name/gpx_number and label_font/color.
   */
  QList<QgsAnnotationItem *> translateKadasGpxRouteItem( const QDomElement &itemEl, const QgsCoordinateReferenceSystem &itemCrs, const QgsCoordinateReferenceSystem &layerCrs )
  {
    QList<QgsAnnotationItem *> out;
    // Reuse the line translator: GpxRoute is just a KadasLineItem with
    // extra label metadata. The shared logic handles geometry parsing,
    // CRS transform, and per-part fanning.
    const QList<QgsAnnotationItem *> baseAnnos = translateKadasLineItem( itemEl, itemCrs, layerCrs );
    if ( baseAnnos.isEmpty() )
      return out;
    const QString gpxName = itemEl.attribute( QStringLiteral( "gpx_name" ) );
    const QString gpxNumber = itemEl.attribute( QStringLiteral( "gpx_number" ) );
    const QString fontStr = itemEl.attribute( QStringLiteral( "label_font" ) );
    const QString colorStr = itemEl.attribute( QStringLiteral( "label_color" ) );
    QFont labelFont;
    if ( !fontStr.isEmpty() )
      labelFont.fromString( fontStr );
    const QColor labelColor = colorStr.isEmpty() ? QColor() : QColor( colorStr );

    for ( QgsAnnotationItem *base : baseAnnos )
    {
      // The line translator returns QgsAnnotationLineItem; promote to
      // KadasGpxRouteAnnotationItem by cloning the geometry + symbol.
      auto *lineAnno = static_cast<QgsAnnotationLineItem *>( base );
      auto *route = new KadasGpxRouteAnnotationItem( static_cast<QgsCurve *>( lineAnno->geometry()->clone() ) );
      if ( lineAnno->symbol() )
        route->setSymbol( lineAnno->symbol()->clone() );
      route->setName( gpxName );
      route->setNumber( gpxNumber );
      route->setLabelFont( labelFont );
      if ( labelColor.isValid() )
        route->setLabelColor( labelColor );
      delete lineAnno;
      out.append( route );
    }
    return out;
  }

  /**
   * Translate one `<MapItem name="KadasGpxWaypointItem">` (v2 format)
   * into a `KadasGpxWaypointAnnotationItem`. Carries gpx_name and
   * label_font/color.
   */
  KadasGpxWaypointAnnotationItem *translateKadasGpxWaypointItem( const QDomElement &itemEl, const QgsCoordinateReferenceSystem &itemCrs, const QgsCoordinateReferenceSystem &layerCrs )
  {
    // Reuse the point translator: GpxWaypoint is just a KadasPointItem
    // with extra label metadata.
    QgsAnnotationMarkerItem *baseAnno = translateKadasPointItem( itemEl, itemCrs, layerCrs );
    if ( !baseAnno )
      return nullptr;
    auto *wp = new KadasGpxWaypointAnnotationItem( QgsPoint( baseAnno->geometry() ) );
    delete baseAnno;
    wp->setName( itemEl.attribute( QStringLiteral( "gpx_name" ) ) );
    const QString fontStr = itemEl.attribute( QStringLiteral( "label_font" ) );
    if ( !fontStr.isEmpty() )
    {
      QFont labelFont;
      labelFont.fromString( fontStr );
      wp->setLabelFont( labelFont );
    }
    const QString colorStr = itemEl.attribute( QStringLiteral( "label_color" ) );
    if ( !colorStr.isEmpty() )
      wp->setLabelColor( QColor( colorStr ) );
    return wp;
  }

  /**
   * Dispatcher: looks at the `name` attribute of \a itemEl and forwards to
   * the matching per-type translator. Returns an empty list if no
   * translator is registered for the type yet, or if the translator
   * itself failed.
   *
   * Common item-level attributes (z_index) are applied here so per-type
   * translators don't repeat the boilerplate. Items that fan one MapItem
   * into multiple annotations (Line, Polygon, ...) all share the parent's
   * z_index.
   */
  QList<QgsAnnotationItem *> translateMapItem( const QDomElement &itemEl, const QgsCoordinateReferenceSystem &layerCrs )
  {
    const QString name = itemEl.attribute( QStringLiteral( "name" ) );
    const QgsCoordinateReferenceSystem itemCrs( itemEl.attribute( QStringLiteral( "crs" ) ) );

    QList<QgsAnnotationItem *> annos;
    if ( name == QLatin1String( "KadasPointItem" ) )
    {
      if ( auto *a = translateKadasPointItem( itemEl, itemCrs, layerCrs ) )
        annos.append( a );
    }
    else if ( name == QLatin1String( "KadasCoordinateCrossItem" ) )
    {
      if ( auto *a = translateKadasCoordinateCrossItem( itemEl, itemCrs, layerCrs ) )
        annos.append( a );
    }
    else if ( name == QLatin1String( "KadasTextItem" ) )
    {
      if ( auto *a = translateKadasTextItem( itemEl, itemCrs, layerCrs ) )
        annos.append( a );
    }
    else if ( name == QLatin1String( "KadasLineItem" ) )
    {
      annos = translateKadasLineItem( itemEl, itemCrs, layerCrs );
    }
    else if ( name == QLatin1String( "KadasPolygonItem" ) )
    {
      annos = translateKadasPolygonItem( itemEl, itemCrs, layerCrs );
    }
    else if ( name == QLatin1String( "KadasRectangleItem" ) )
    {
      annos = translateKadasRectangleItem( itemEl, itemCrs, layerCrs );
    }
    else if ( name == QLatin1String( "KadasCircleItem" ) )
    {
      annos = translateKadasCircleItem( itemEl, itemCrs, layerCrs );
    }
    else if ( name == QLatin1String( "KadasPictureItem" ) )
    {
      if ( auto *a = translateKadasPictureItem( itemEl, itemCrs, layerCrs ) )
        annos.append( a );
    }
    else if ( name == QLatin1String( "KadasSymbolItem" ) )
    {
      if ( auto *a = translateKadasSymbolItem( itemEl, itemCrs, layerCrs ) )
        annos.append( a );
    }
    else if ( name == QLatin1String( "KadasPinItem" ) )
    {
      if ( auto *a = translateKadasPinItem( itemEl, itemCrs, layerCrs ) )
        annos.append( a );
    }
    else if ( name == QLatin1String( "KadasCircularSectorItem" ) )
    {
      annos = translateKadasCircularSectorItem( itemEl, itemCrs, layerCrs );
    }
    else if ( name == QLatin1String( "KadasGpxRouteItem" ) )
    {
      annos = translateKadasGpxRouteItem( itemEl, itemCrs, layerCrs );
    }
    else if ( name == QLatin1String( "KadasGpxWaypointItem" ) )
    {
      if ( auto *a = translateKadasGpxWaypointItem( itemEl, itemCrs, layerCrs ) )
        annos.append( a );
    }

    if ( annos.isEmpty() )
      return annos;

    bool ok = false;
    const int z = itemEl.attribute( QStringLiteral( "z_index" ), QStringLiteral( "0" ) ).toInt( &ok );
    if ( ok )
    {
      for ( QgsAnnotationItem *a : annos )
        a->setZIndex( z );
    }

    return annos;
  }
} // namespace

bool KadasProjectMigration::migrateLegacyKadasItemLayers( QDomDocument &doc, QDomElement &root )
{
  QDomElement projectLayersEl = root.firstChildElement( QStringLiteral( "projectlayers" ) );
  if ( projectLayersEl.isNull() )
    return false;

  // Snapshot direct children only — nested `<maplayer>` blocks inside
  // `<originalStyle>` etc. must not be touched.
  QList<QDomElement> itemMapLayers;
  for ( QDomNode n = projectLayersEl.firstChild(); !n.isNull(); n = n.nextSibling() )
  {
    QDomElement el = n.toElement();
    if ( el.tagName() == QLatin1String( "maplayer" ) && el.attribute( QStringLiteral( "type" ) ) == QLatin1String( "plugin" ) && el.attribute( QStringLiteral( "name" ) ) == QLatin1String( "KadasItemLayer" ) )
    {
      itemMapLayers.append( el );
    }
  }
  if ( itemMapLayers.isEmpty() )
    return false;

  bool anyRewritten = false;

  for ( QDomElement &mapLayerEl : itemMapLayers )
  {
    const QString originalId = mapLayerEl.firstChildElement( QStringLiteral( "id" ) ).text();
    const QString originalName = mapLayerEl.firstChildElement( QStringLiteral( "layername" ) ).text();
    const QString originalTitle = mapLayerEl.attribute( QStringLiteral( "title" ), originalName );
    const QString srsAuthid = mapLayerEl.firstChildElement( QStringLiteral( "srs" ) ).firstChildElement( QStringLiteral( "spatialrefsys" ) ).firstChildElement( QStringLiteral( "authid" ) ).text();
    QgsCoordinateReferenceSystem layerCrs( srsAuthid );
    if ( !layerCrs.isValid() )
      layerCrs = QgsCoordinateReferenceSystem( QStringLiteral( "EPSG:3857" ) );

    // First pass: translate every MapItem into a QgsAnnotationItem. If any
    // single translation fails, abandon this layer (post-load fallback
    // will handle it as a legacy KadasItemLayer).
    QList<QgsAnnotationItem *> translated;
    QStringList tooltips;
    bool allOk = true;
    for ( QDomNode itemNode = mapLayerEl.firstChild(); !itemNode.isNull(); itemNode = itemNode.nextSibling() )
    {
      QDomElement itemEl = itemNode.toElement();
      if ( itemEl.isNull() || itemEl.tagName() != QLatin1String( "MapItem" ) )
        continue;

      // Rewrite legacy v1 (JSON-CDATA) payloads to the v2 attribute
      // layout in place; v2 elements pass through unchanged.
      convertV1MapItemToV2( itemEl );

      QList<QgsAnnotationItem *> annos = translateMapItem( itemEl, layerCrs );
      if ( annos.isEmpty() )
      {
        QgsDebugMsgLevel( QStringLiteral( "KadasItemLayer XML rewrite skipped for layer '%1': item type '%2' not translatable yet" ).arg( originalName, itemEl.attribute( QStringLiteral( "name" ) ) ), 1 );
        allOk = false;
        break;
      }
      const QString tooltip = itemEl.attribute( QStringLiteral( "tooltip" ) );
      for ( QgsAnnotationItem *a : annos )
      {
        translated.append( a );
        // Pins render their tooltip from the current title/description
        // (see KadasMapToolEditAnnotationItem), so synthesize it here
        // rather than importing 2.x's frozen position/height HTML.
        if ( const auto *pin = dynamic_cast<const KadasPinAnnotationItem *>( a ) )
        {
          QString html;
          if ( !pin->name().isEmpty() )
            html += QStringLiteral( "<b>%1</b>" ).arg( pin->name().toHtmlEscaped() );
          if ( !pin->remarks().isEmpty() )
          {
            if ( !html.isEmpty() )
              html += QStringLiteral( "<br>" );
            html += pin->remarks().toHtmlEscaped().replace( '\n', QStringLiteral( "<br>" ) );
          }
          tooltips.append( html );
        }
        else
        {
          tooltips.append( tooltip );
        }
      }
    }

    if ( !allOk )
    {
      qDeleteAll( translated );
      continue;
    }

    // Build a temporary QgsAnnotationLayer, populate, and let QGIS write
    // its own XML. Splicing the result back preserves the original layer
    // id and layername so layer-tree-layer references continue to resolve.
    QgsAnnotationLayer::LayerOptions options( QgsProject::instance()->transformContext() );
    auto annoLayer = std::make_unique<QgsAnnotationLayer>( originalName, options );
    annoLayer->setCrs( layerCrs );
    for ( int i = 0; i < translated.size(); ++i )
    {
      const QString newId = annoLayer->addItem( translated[i] );
      if ( !tooltips[i].isEmpty() )
        KadasAnnotationLayerHelpers::setTooltip( annoLayer.get(), newId, tooltips[i] );

      // QgsAnnotationItem::writeCommonProperties drops offsetFromCallout
      // values where QSizeF::isValid() is false (any negative dimension).
      // Picture items frequently use negative offsets (legacy convention
      // for "image to the left of the anchor") so we'd lose the placement
      // on serialization. Mirror what
      // KadasAnnotationProjectIntegration::prepareForSave() does on save:
      // shadow the offset into the layer's customProperty so the load
      // path's readPictureOffsetsFromCustomProperties() can restore it.
      if ( auto *pic = dynamic_cast<QgsAnnotationPictureItem *>( translated[i] ) )
      {
        const QSizeF off = pic->offsetFromCallout();
        if ( off.width() != 0 || off.height() != 0 )
        {
          annoLayer->setCustomProperty( QStringLiteral( "kadas:picture-offset:" ) + newId, QgsSymbolLayerUtils::encodeSize( off ) );
          annoLayer->setCustomProperty( QStringLiteral( "kadas:picture-offset-unit:" ) + newId, QgsUnitTypes::encodeUnit( pic->offsetFromCalloutUnit() ) );
        }
      }
    }

    QDomElement newMapLayerEl = doc.createElement( QStringLiteral( "maplayer" ) );
    QgsReadWriteContext context;
    annoLayer->writeLayerXml( newMapLayerEl, doc, context );

    // Restore original id and layername (writeLayerXml emits freshly
    // generated ones).
    QDomElement existingId = newMapLayerEl.firstChildElement( QStringLiteral( "id" ) );
    if ( !existingId.isNull() )
    {
      QDomElement replacementId = doc.createElement( QStringLiteral( "id" ) );
      replacementId.appendChild( doc.createTextNode( originalId ) );
      newMapLayerEl.replaceChild( replacementId, existingId );
    }
    QDomElement existingName = newMapLayerEl.firstChildElement( QStringLiteral( "layername" ) );
    if ( !existingName.isNull() )
    {
      QDomElement replacementName = doc.createElement( QStringLiteral( "layername" ) );
      replacementName.appendChild( doc.createTextNode( originalName ) );
      newMapLayerEl.replaceChild( replacementName, existingName );
    }

    projectLayersEl.replaceChild( newMapLayerEl, mapLayerEl );
    anyRewritten = true;
  }

  return anyRewritten;
}
