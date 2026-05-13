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

#include <cmath>

#include <qgis/qgsannotationlayer.h>
#include <qgis/qgscircularstring.h>
#include <qgis/qgslinestring.h>
#include <qgis/qgslogger.h>
#include <qgis/qgspoint.h>
#include <qgis/qgspolygon.h>
#include <qgis/qgsproject.h>
#include <qgis/qgssymbollayerutils.h>

#include "kadas/core/kadas.h"
#include "kadas/gui/annotationitems/kadasannotationlayerhelpers.h"
#include "kadas/gui/annotationitems/kadasmilxannotationitem.h"
#include "kadas/gui/annotationitems/kadasmilxlayersettings.h"
#include "kadas/gui/kadasitemlayer.h"
#include "kadas/gui/kadasprojectmigration.h"
#include "kadas/gui/milx/kadasmilxclient.h"
#include "kadas/gui/mapitems/kadascircleitem.h"
#include "kadas/gui/mapitems/kadasgpxrouteitem.h"
#include "kadas/gui/mapitems/kadasgpxwaypointitem.h"
#include "kadas/gui/mapitems/kadaspictureitem.h"
#include "kadas/gui/mapitems/kadaslineitem.h"
#include "kadas/gui/mapitems/kadaspointitem.h"
#include "kadas/gui/mapitems/kadaspolygonitem.h"
#include "kadas/gui/mapitems/kadasrectangleitem.h"
#include "kadas/gui/mapitems/kadassymbolitem.h"
#include "kadas/gui/mapitems/kadastextitem.h"

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

    // Write the mutated XML back over the unzipped `.qgs`, then repack
    // the whole archive into a new temp `.qgz`. Keep the temp dir alive
    // until QGIS has finished reading the archive by transferring its
    // ownership through a static list — `QgsArchive` deletes its
    // backing temp dir in the destructor.
    if ( !qgsFile.open( QIODevice::WriteOnly | QIODevice::Truncate ) )
      return fileName;
    qgsFile.write( doc.toString().toUtf8() );
    qgsFile.close();

    QTemporaryFile outFile( QDir::tempPath() + QStringLiteral( "/kadas-migrated-XXXXXX.qgz" ) );
    outFile.setAutoRemove( false );
    if ( !outFile.open() )
      return fileName;
    const QString outPath = outFile.fileName();
    outFile.close();
    QFile::remove( outPath ); // QgsArchive::zip refuses to overwrite an existing file
    if ( !archive->zip( outPath ) )
      return fileName;
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

  if ( root.attribute( "version" ) == "2.15.2-KADAS" )
  {
    migrateKadas1xTo2x( doc, root, basedir, filesToAttach );
    // Kadas 1.x → 2.x produces fresh annotation layers for MilX, so the
    // legacy KadasMilxLayer rewrite below is a no-op for these projects.
    return true;
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
  // all be translated by this pass are left as plugin layers and the
  // post-load `KadasItemLayerMigration` fallback handles them.
  changed = migrateLegacyKadasItemLayers( doc, root ) || changed;
  return changed;
}

void KadasProjectMigration::migrateKadas1xTo2x( QDomDocument &doc, QDomElement &root, const QString &basedir, QStringList &filesToAttach )
{
  // Datasource of map layers
  QDomElement projectLayersEl = root.firstChildElement( "projectlayers" );
  QDomNodeList maplayers = projectLayersEl.elementsByTagName( "maplayer" );
  for ( int i = 0, n = maplayers.size(); i < n; ++i )
  {
    QDomElement maplayer = maplayers.at( i ).toElement();
    QDomElement datasourceEl = maplayer.firstChildElement( "datasource" );
    QString datasourceText = datasourceEl.text();

    // If datasource is relative to basedir, mark it as to be attached
    if ( shouldAttach( basedir, datasourceText ) )
    {
      QString fullPath = QDir( basedir ).absoluteFilePath( datasourceText );
      QDomElement newDatasourceEl = doc.createElement( "datasource" );
      newDatasourceEl.appendChild( doc.createTextNode( fullPath ) );
      maplayer.replaceChild( newDatasourceEl, datasourceEl );
      filesToAttach.append( fullPath );
    }
  }

  // Redlining / GPS routes: rewrite completely as item layers
  QString redliningLayerId = root.firstChildElement( "Redlining" ).attribute( "layerid" );
  QString gpsRoutesLayerId = root.firstChildElement( "GpsRoutes" ).attribute( "layerid" );
  root.removeChild( root.firstChildElement( "Redlining" ) );
  root.removeChild( root.firstChildElement( "GpsRoutes" ) );
  for ( int i = 0, n = maplayers.size(); i < n; ++i )
  {
    QDomElement mapLayerEl = maplayers.at( i ).toElement();
    if ( mapLayerEl.attribute( "type" ) != "redlining" )
    {
      continue;
    }
    bool isGps = mapLayerEl.firstChildElement( "id" ).text() == gpsRoutesLayerId;

    QDomElement newMapLayerEl = doc.createElement( "maplayer" );
    newMapLayerEl.setAttribute( "name", "KadasItemLayer" );
    newMapLayerEl.setAttribute( "type", "plugin" );
    newMapLayerEl.setAttribute( "title", mapLayerEl.firstChildElement( "layername" ).text() );

    QDomElement idEl = doc.createElement( "id" );
    idEl.appendChild( doc.createTextNode( mapLayerEl.firstChildElement( "id" ).text() ) );
    newMapLayerEl.appendChild( idEl );

    QDomElement layerNameEl = doc.createElement( "layername" );
    layerNameEl.appendChild( doc.createTextNode( mapLayerEl.firstChildElement( "layername" ).text() ) );
    newMapLayerEl.appendChild( layerNameEl );

    newMapLayerEl.appendChild( mapLayerEl.firstChildElement( "srs" ).cloneNode() );
    QgsCoordinateReferenceSystem crs( mapLayerEl.firstChildElement( "srs" ).firstChildElement( "spatialrefsys" ).firstChildElement( "authid" ).text() );

    newMapLayerEl.appendChild( mapLayerEl.firstChildElement( "globe" ).cloneNode() );

    QDomNodeList redliningItems = mapLayerEl.elementsByTagName( "RedliningItem" );
    for ( int i = 0, n = redliningItems.size(); i < n; ++i )
    {
      QDomElement redliningItemEl = redliningItems.at( i ).toElement();
      QMap<QString, QString> flags = deserializeLegacyRedliningFlags( redliningItemEl.attribute( "flags" ) );

      KadasMapItem *item = nullptr;
      if ( !redliningItemEl.attribute( "text" ).isEmpty() && flags["shape"] == "point" && flags["symbol"].isEmpty() )
      {
        QgsPoint point;
        point.fromWkt( redliningItemEl.attribute( "geometry" ) );
        QColor outline = QgsSymbolLayerUtils::decodeColor( redliningItemEl.attribute( "outline" ) );
        outline.setAlpha( 255 ); // KADAS 1 ignored outline transparency;
        QColor fill = QgsSymbolLayerUtils::decodeColor( redliningItemEl.attribute( "fill" ) );
        fill.setAlpha( 255 ); // KADAS 1 ignored fill transparency;

        KadasTextItem *textItem = new KadasTextItem( crs );
        textItem->setText( redliningItemEl.attribute( "text" ) );
        textItem->setPosition( KadasItemPos::fromPoint( point ) );
        //textItem->setAngle( flags["rotation"].toDouble() );
        //textItem->setOutlineColor( outline );
        textItem->setColor( fill );
        // textItem->state()->mRectangleCenterPoint.setX( 0 );
        // textItem->state()->mRectangleCenterPoint.setY( 1. );

        QFont font;
        font.setBold( flags["bold"].toInt() != 0 );
        font.setFamily( flags["family"] );
        font.setPointSize( flags["fontSize"].toInt() );
        font.setItalic( flags["italic"].toInt() != 0 );
        textItem->setFont( font );

        item = textItem;
      }
      else
      {
        if ( flags["shape"] == "point" )
        {
          KadasPointItem *pointItem = nullptr;
          const QgsPointXY point = QgsGeometry::fromWkt( redliningItemEl.attribute( "geometry" ) ).asPoint();

          if ( isGps )
          {
            pointItem = new KadasGpxWaypointItem();
            static_cast<KadasGpxWaypointItem *>( pointItem )->setName( redliningItemEl.attribute( "text" ) );
          }
          else
          {
            Qgis::MarkerShape iconType = Qgis::MarkerShape::Circle;
            if ( flags["symbol"] == "circle" )
            {
              iconType = Qgis::MarkerShape::Circle;
            }
            else if ( flags["symbol"] == "rectangle" )
            {
              iconType = Qgis::MarkerShape::Square;
            }
            else if ( flags["symbol"] == "triangle" )
            {
              iconType = Qgis::MarkerShape::Triangle;
            }

            pointItem = new KadasPointItem( crs, iconType );
          }

          pointItem->setPoint( QgsPoint( point ) );

          pointItem->setIconSize( 8 * redliningItemEl.attribute( "size" ).toInt() );
          pointItem->setColor( QgsSymbolLayerUtils::decodeColor( redliningItemEl.attribute( "fill" ) ) );
          pointItem->setStrokeColor( QgsSymbolLayerUtils::decodeColor( redliningItemEl.attribute( "outline" ) ) );
          pointItem->setStrokeWidth( redliningItemEl.attribute( "size" ).toInt() );

          item = pointItem;
        }
        else
        {
          QgsAbstractGeometry *geom = nullptr;

          KadasGeometryItem *geomItem = nullptr;
          if ( flags["shape"] == "line" )
          {
            geom = new QgsLineString();
            geom->fromWkt( redliningItemEl.attribute( "geometry" ) );
            if ( isGps )
            {
              geomItem = new KadasGpxRouteItem();
              static_cast<KadasGpxRouteItem *>( geomItem )->setName( redliningItemEl.attribute( "text" ) );
              static_cast<KadasGpxRouteItem *>( geomItem )->setNumber( flags["routeNumber"] );
            }
            else
            {
              geomItem = new KadasLineItem( crs );
            }
          }
          else if ( flags["shape"] == "polygon" )
          {
            geom = new QgsPolygon();
            geom->fromWkt( redliningItemEl.attribute( "geometry" ) );

            geomItem = new KadasPolygonItem( crs );
          }
          else if ( flags["shape"] == "rectangle" )
          {
            geom = new QgsPolygon();
            geom->fromWkt( redliningItemEl.attribute( "geometry" ) );

            geomItem = new KadasRectangleItem( crs );
          }
          else if ( flags["shape"] == "circle" )
          {
            // Circular strings were incorrectly serialized to wkt as ringpoint - center - ringpoint instead of 3x ringpoint
            QgsCurvePolygon curve;
            curve.fromWkt( redliningItemEl.attribute( "geometry" ) );
            QgsPointSequence points;
            curve.exteriorRing()->points( points );
            if ( points.size() == 3 )
            {
              points[1].setX( points[1].x() - points[0].distance( points[1] ) );
            }

            QgsCircularString *ring = new QgsCircularString();
            ring->setPoints( points );
            geom = new QgsCurvePolygon();
            static_cast<QgsCurvePolygon *>( geom )->setExteriorRing( ring );

            geomItem = new KadasCircleItem( crs );
          }

          if ( geomItem && geom )
          {
            geomItem->addPartFromGeometry( *geom );

            QBrush brush;
            brush.setColor( QgsSymbolLayerUtils::decodeColor( redliningItemEl.attribute( "fill" ) ) );
            brush.setStyle( QgsSymbolLayerUtils::decodeBrushStyle( redliningItemEl.attribute( "fill_style" ) ) );
            geomItem->setFill( brush );

            QPen pen;
            pen.setColor( QgsSymbolLayerUtils::decodeColor( redliningItemEl.attribute( "outline" ) ) );
            pen.setStyle( QgsSymbolLayerUtils::decodePenStyle( redliningItemEl.attribute( "outline_style" ) ) );
            pen.setWidth( redliningItemEl.attribute( "size" ).toInt() );
            geomItem->setOutline( pen );

            geomItem->setIconSize( 8 * pen.width() );
            geomItem->setIconFill( brush );
            geomItem->setIconOutline( QPen( pen.color(), pen.width(), pen.style() ) );

            item = geomItem;
          }
          delete geom;
        }
      }
      if ( !item )
      {
        continue;
      }

      newMapLayerEl.appendChild( item->writeXml( doc ) );
      delete item;
    }
    root.firstChildElement( "projectlayers" ).replaceChild( newMapLayerEl, mapLayerEl );
  }

  // SVGAnnotationItem: Convert layer and items
  QDomNodeList svgItems = root.elementsByTagName( "SVGAnnotationItem" );
  if ( !svgItems.isEmpty() )
  {
    QMap<QString, QDomElement> newMapLayerEls;
    for ( int i = 0, n = svgItems.size(); i < n; ++i )
    {
      QDomElement svgItemEl = svgItems.at( i ).toElement();
      QDomElement annotationItemEl = svgItemEl.firstChildElement( "AnnotationItem" );
      QString layerId = annotationItemEl.attribute( "layerId" );

      if ( !newMapLayerEls.contains( layerId ) )
      {
        newMapLayerEls[layerId] = replaceAnnotationLayer( doc, root, layerId );
      }
      QDomElement newMapLayerEl = newMapLayerEls[layerId];
      if ( newMapLayerEl.isNull() )
      {
        continue;
      }

      // If file is relative to the basedir, mark it as to be attached
      QString fileName = svgItemEl.attribute( "file" );
      if ( shouldAttach( basedir, fileName ) )
      {
        fileName = QDir( basedir ).absoluteFilePath( fileName );
        filesToAttach.append( fileName );
      }
      int width = annotationItemEl.attribute( "frameWidth" ).toInt();
      int height = annotationItemEl.attribute( "frameHeight" ).toInt();
      double angle = annotationItemEl.attribute( "angle" ).toDouble();
      double adjWidth = height * qAbs( std::sin( angle / 180. * M_PI ) ) + width * qAbs( std::cos( angle / 180. * M_PI ) );
      double adjHeight = height * qAbs( std::cos( angle / 180. * M_PI ) ) + width * qAbs( std::sin( angle / 180. * M_PI ) );
      double scale = std::min( width / adjWidth, height / adjHeight );

      KadasSymbolItem symbolItem( ( QgsCoordinateReferenceSystem( annotationItemEl.attribute( "mapGeoPosAuthID" ) ) ) );
      symbolItem.setup( fileName, 0.5, 0.5, width * scale );
      symbolItem.setPosition( KadasItemPos::fromPoint( QgsPointXY( annotationItemEl.attribute( "geoPosX" ).toDouble(), annotationItemEl.attribute( "geoPosY" ).toDouble() ) ) );
      symbolItem.setAngle( -angle );

      newMapLayerEl.appendChild( symbolItem.writeXml( doc ) );
    }
  }

  // GeoImageAnnotationItem: Convert layer and items
  QDomNodeList imageItems = root.elementsByTagName( "GeoImageAnnotationItem" );
  if ( !imageItems.isEmpty() )
  {
    QMap<QString, QDomElement> newMapLayerEls;
    for ( int i = 0, n = imageItems.size(); i < n; ++i )
    {
      QDomElement imageItemEl = imageItems.at( i ).toElement();
      QDomElement annotationItemEl = imageItemEl.firstChildElement( "AnnotationItem" );
      QString layerId = annotationItemEl.attribute( "layerId" );

      if ( !newMapLayerEls.contains( layerId ) )
      {
        newMapLayerEls[layerId] = replaceAnnotationLayer( doc, root, layerId );
      }
      QDomElement newMapLayerEl = newMapLayerEls[layerId];
      if ( newMapLayerEl.isNull() )
      {
        continue;
      }

      // If file is relative to the basedir, mark it as to be attached
      QString fileName = imageItemEl.attribute( "file" );
      if ( shouldAttach( basedir, fileName ) )
      {
        fileName = QDir( basedir ).absoluteFilePath( fileName );
        filesToAttach.append( fileName );
      }

      QgsPointXY pos( annotationItemEl.attribute( "geoPosX" ).toDouble(), annotationItemEl.attribute( "geoPosY" ).toDouble() );
      int width = annotationItemEl.attribute( "frameWidth" ).toInt();
      int height = annotationItemEl.attribute( "frameHeight" ).toInt();
      int offsetX = -annotationItemEl.attribute( "offsetX" ).toInt() - 0.5 * width;
      int offsetY = -annotationItemEl.attribute( "offsetY" ).toInt() - 0.5 * height;

      KadasPictureItem pictureItem( ( QgsCoordinateReferenceSystem( annotationItemEl.attribute( "mapGeoPosAuthID" ) ) ) );
      pictureItem.setup( fileName, KadasItemPos::fromPoint( pos ), true, offsetX, offsetY, width );

      newMapLayerEl.appendChild( pictureItem.writeXml( doc ) );
    }
  }

  // PinAnnotationItem
  QDomNodeList pinItems = root.elementsByTagName( "PinAnnotationItem" );
  if ( !pinItems.isEmpty() )
  {
    QMap<QString, QDomElement> newMapLayerEls;
    for ( int i = 0, n = pinItems.size(); i < n; ++i )
    {
      QDomElement pinItemEl = pinItems.at( i ).toElement();
      QDomElement annotationItemEl = pinItemEl.firstChildElement( "AnnotationItem" );
      QString layerId = annotationItemEl.attribute( "layerId" );

      if ( !newMapLayerEls.contains( layerId ) )
      {
        newMapLayerEls[layerId] = replaceAnnotationLayer( doc, root, layerId );
      }
      QDomElement newMapLayerEl = newMapLayerEls[layerId];
      if ( newMapLayerEl.isNull() )
      {
        continue;
      }

      KadasPinItem pinItem( ( QgsCoordinateReferenceSystem( annotationItemEl.attribute( "mapGeoPosAuthID" ) ) ) );
      QgsPointXY pos( annotationItemEl.attribute( "geoPosX" ).toDouble(), annotationItemEl.attribute( "geoPosY" ).toDouble() );
      pinItem.setPosition( KadasItemPos::fromPoint( pos ) );
      pinItem.setName( pinItemEl.attribute( "pinName" ) );
      pinItem.setRemarks( pinItemEl.firstChildElement( "PinRemarks" ).text() );

      newMapLayerEl.appendChild( pinItem.writeXml( doc ) );
    }
  }

  // MilX
  for ( int i = 0, n = maplayers.size(); i < n; ++i )
  {
    QDomElement mapLayerEl = maplayers.at( i ).toElement();
    if ( mapLayerEl.attribute( "type" ) != "plugin" || mapLayerEl.attribute( "name" ) != "MilX_Layer" )
    {
      continue;
    }
    const QString layerName = mapLayerEl.firstChildElement( "layername" ).text();
    const int dpi = qApp->primaryScreen()->logicalDotsPerInchX();

    QgsAnnotationLayer::LayerOptions options( QgsProject::instance()->transformContext() );
    auto annoLayer = std::make_unique<QgsAnnotationLayer>( layerName, options );
    annoLayer->setCrs( QgsCoordinateReferenceSystem( "EPSG:4326" ) );

    QString err;
    KadasMilxAnnotationItem::importLayerFromMilxly( annoLayer.get(), mapLayerEl.firstChildElement( "MilXLayer" ), dpi, QgsProject::instance()->transformContext(), err );

    QDomElement newMapLayerEl = doc.createElement( "maplayer" );
    QgsReadWriteContext context;
    annoLayer->writeLayerXml( newMapLayerEl, doc, context );
    newMapLayerEl.appendChild( mapLayerEl.firstChildElement( "id" ).cloneNode() );
    newMapLayerEl.appendChild( mapLayerEl.firstChildElement( "layername" ).cloneNode() );

    projectLayersEl.replaceChild( newMapLayerEl, mapLayerEl );
  }

  // Changes to guide grid etc?
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
// are deliberately not handled here: they are recognised by
// `format_version != "2"` and cause the translator to return null, leaving
// the whole layer untouched so the post-load `KadasItemLayerMigration`
// fallback (which still understands v1) takes over.
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
#include "kadas/gui/annotationitems/kadaspictureannotationcontroller.h"
#include "kadas/gui/annotationitems/kadaspinannotationitem.h"
#include "kadas/gui/annotationitems/kadasgpxrouteannotationitem.h"
#include "kadas/gui/annotationitems/kadasgpxwaypointannotationitem.h"

namespace
{
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
    symbolLayer->setStrokeColor( strokeColor );
    symbolLayer->setColor( fillColor );

    auto *anno = new QgsAnnotationMarkerItem( QgsPoint( pt ) );
    anno->setSymbol( new QgsMarkerSymbol( QgsSymbolLayerList { symbolLayer } ) );
    return anno;
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
      QgsPointXY p1 = p1List[i];
      QgsPointXY p2 = p2List[i];
      if ( needTransform )
      {
        try
        {
          p1 = ct.transform( p1 );
          p2 = ct.transform( p2 );
        }
        catch ( QgsCsException & )
        {
          qDeleteAll( out );
          out.clear();
          return out;
        }
      }
      const QgsPointXY center( 0.5 * ( p1.x() + p2.x() ), 0.5 * ( p1.y() + p2.y() ) );
      const QSizeF size( std::abs( p2.x() - p1.x() ), std::abs( p2.y() - p1.y() ) );
      auto *anno = new KadasRectangleAnnotationItem( center, size, 0.0 );
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

    auto *pic = new QgsAnnotationPictureItem( Qgis::PictureFormat::Unknown, QString(), QgsRectangle( anchor.x(), anchor.y(), anchor.x(), anchor.y() ) );
    if ( !filePath.isEmpty() )
      KadasPictureAnnotationController::setPath( pic, filePath );
    pic->setPlacementMode( Qgis::AnnotationPlacementMode::FixedSize );
    pic->setFixedSize( QSizeF( w > 0 ? w : 200, h > 0 ? h : 150 ) );
    pic->setFixedSizeUnit( Qgis::RenderUnit::Pixels );
    pic->setFrameEnabled( frame );
    KadasPictureAnnotationController::ensureBalloon( pic );
    // Override the centered default that ensureBalloon installs with the
    // user's saved offset (in pixels, same convention as the legacy item).
    pic->setOffsetFromCallout( QSizeF( offsetX, offsetY ) );
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

    auto *pic = new QgsAnnotationPictureItem( Qgis::PictureFormat::Unknown, QString(), QgsRectangle( anchor.x(), anchor.y(), anchor.x(), anchor.y() ) );
    if ( !filePath.isEmpty() )
      KadasPictureAnnotationController::setPath( pic, filePath );
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
    anno->setName( itemEl.attribute( QStringLiteral( "name" ) ) );
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
        tooltips.append( tooltip );
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
