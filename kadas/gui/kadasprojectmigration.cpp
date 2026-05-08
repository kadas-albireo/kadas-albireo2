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

#include <qgis/qgsannotationlayer.h>
#include <qgis/qgscircularstring.h>
#include <qgis/qgslinestring.h>
#include <qgis/qgslogger.h>
#include <qgis/qgspoint.h>
#include <qgis/qgspolygon.h>
#include <qgis/qgsproject.h>
#include <qgis/qgssymbollayerutils.h>

#include "kadas/core/kadas.h"
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
    qgsFile.write( doc.toString().toLocal8Bit() );
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
      tempFile.write( doc.toString().toLocal8Bit() );
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
  return migrateLegacyMilxLayers( doc, root );
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

        textItem->setEditor( "KadasRedliningTextEditor" );

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
            pointItem->setEditor( "KadasRedliningItemEditor" );
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
              geomItem->setEditor( "KadasRedliningItemEditor" );
            }
          }
          else if ( flags["shape"] == "polygon" )
          {
            geom = new QgsPolygon();
            geom->fromWkt( redliningItemEl.attribute( "geometry" ) );

            geomItem = new KadasPolygonItem( crs );
            geomItem->setEditor( "KadasRedliningItemEditor" );
          }
          else if ( flags["shape"] == "rectangle" )
          {
            geom = new QgsPolygon();
            geom->fromWkt( redliningItemEl.attribute( "geometry" ) );

            geomItem = new KadasRectangleItem( crs );
            geomItem->setEditor( "KadasRedliningItemEditor" );
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
            geomItem->setEditor( "KadasRedliningItemEditor" );
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
      pinItem.setEditor( "KadasSymbolAttributesEditor" );
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
    for ( QDomNode itemNode = mapLayerEl.firstChild(); !itemNode.isNull(); itemNode = itemNode.nextSibling() )
    {
      QDomElement itemEl = itemNode.toElement();
      if ( itemEl.tagName() != QLatin1String( "MapItem" ) || itemEl.attribute( QStringLiteral( "name" ) ) != QLatin1String( "KadasMilxItem" ) )
        continue;

      const QJsonObject data = QJsonDocument::fromJson( itemEl.firstChild().toCDATASection().data().toLocal8Bit() ).object();
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

      annoLayer->addItem( anno );
    }

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
