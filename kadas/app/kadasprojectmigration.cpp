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

#include <QDir>
#include <QDomDocument>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMap>

#include <qgis/qgscircularstring.h>
#include <qgis/qgslinestring.h>
#include <qgis/qgslogger.h>
#include <qgis/qgspoint.h>
#include <qgis/qgspolygon.h>
#include <qgis/qgsproject.h>
#include <qgis/qgssymbollayerutils.h>

#include <kadas/core/kadas.h>
#include <kadas/gui/kadasitemlayer.h>
#include <kadas/gui/mapitems/kadascircleitem.h>
#include <kadas/gui/mapitems/kadasgpxrouteitem.h>
#include <kadas/gui/mapitems/kadasgpxwaypointitem.h>
#include <kadas/gui/mapitems/kadaspictureitem.h>
#include <kadas/gui/mapitems/kadaslineitem.h>
#include <kadas/gui/mapitems/kadaspointitem.h>
#include <kadas/gui/mapitems/kadaspolygonitem.h>
#include <kadas/gui/mapitems/kadasrectangleitem.h>
#include <kadas/gui/mapitems/kadassymbolitem.h>
#include <kadas/gui/mapitems/kadastextitem.h>
#include <kadas/gui/milx/kadasmilxlayer.h>
#include <kadas/app/kadasapplication.h>
#include <kadas/app/kadasmainwindow.h>
#include <kadas/app/kadasprojectmigration.h>


QString KadasProjectMigration::migrateProject( const QString &fileName, QStringList &filesToAttach )
{
  QFile file( fileName );
  if ( !file.open( QIODevice::ReadOnly ) )
  {
    return fileName;
  }

  QDomDocument doc;
  if ( !doc.setContent( &file ) )
  {
    QgsDebugMsg( "Failed to parse project" );
    return fileName;
  }
  QString basedir = QFileInfo( fileName ).path();

  QDomElement root = doc.documentElement();
  if ( root.tagName() != "qgis" )
  {
    QgsDebugMsg( "Invalid project (incorrect root tag name)" );
  }


  if ( root.attribute( "version" ) == "2.15.2-KADAS" )
  {
    migrateKadas1xTo2x( doc, root, basedir, filesToAttach );
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
        textItem->setAngle( flags["rotation"].toDouble() );
        textItem->setOutlineColor( outline );
        textItem->setFillColor( fill );
        textItem->setAnchorX( 0 );
        textItem->setAnchorY( 1. );

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
        KadasGeometryItem *geomItem = nullptr;
        QgsAbstractGeometry *geom = nullptr;

        if ( flags["shape"] == "point" )
        {
          geom = new QgsPoint();
          geom->fromWkt( redliningItemEl.attribute( "geometry" ) );

          if ( isGps )
          {
            geomItem = new KadasGpxWaypointItem();
            static_cast<KadasGpxWaypointItem *>( geomItem )->setName( redliningItemEl.attribute( "text" ) );
            geomItem->setEditor( "KadasGpxWaypointEditor" );
          }
          else
          {
            KadasPointItem::IconType iconType = KadasPointItem::ICON_CIRCLE;
            if ( flags["symbol"] == "circle" )
            {
              iconType = KadasPointItem::ICON_CIRCLE;
            }
            else if ( flags["symbol"] == "rectangle" )
            {
              iconType = KadasPointItem::ICON_FULL_BOX;
            }
            else if ( flags["symbol"] == "triangle" )
            {
              iconType = KadasPointItem::ICON_FULL_TRIANGLE;
            }

            geomItem = new KadasPointItem( crs, iconType );
            geomItem->setEditor( "KadasRedliningItemEditor" );
          }
        }
        else if ( flags["shape"] == "line" )
        {
          geom = new QgsLineString();
          geom->fromWkt( redliningItemEl.attribute( "geometry" ) );
          if ( isGps )
          {
            geomItem = new KadasGpxRouteItem();
            static_cast<KadasGpxRouteItem *>( geomItem )->setName( redliningItemEl.attribute( "text" ) );
            static_cast<KadasGpxRouteItem *>( geomItem )->setNumber( flags["routeNumber"] );
            geomItem->setEditor( "KadasGpxRouteEditor" );
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

          QgsRectangle bbox = curve.boundingBox();
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
      if ( !item )
      {
        continue;
      }

      QDomElement mapItemEl = doc.createElement( "MapItem" );
      mapItemEl.setAttribute( "name", item->metaObject()->className() );
      mapItemEl.setAttribute( "crs", item->authId() );
      QJsonDocument jsonDoc;
      jsonDoc.setObject( item->serialize() );
      mapItemEl.appendChild( doc.createCDATASection( jsonDoc.toJson( QJsonDocument::Compact ) ) );

      newMapLayerEl.appendChild( mapItemEl );
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
      double adjWidth = height * qAbs( qSin( angle / 180. * M_PI ) ) + width * qAbs( qCos( angle / 180. * M_PI ) );
      double adjHeight = height * qAbs( qCos( angle / 180. * M_PI ) ) + width * qAbs( qSin( angle / 180. * M_PI ) );
      double scale = qMin( width / adjWidth, height / adjHeight );

      KadasSymbolItem symbolItem( ( QgsCoordinateReferenceSystem( annotationItemEl.attribute( "mapGeoPosAuthID" ) ) ) );
      symbolItem.setup( fileName, 0.5, 0.5, width * scale );
      symbolItem.setPosition( KadasItemPos::fromPoint( QgsPointXY( annotationItemEl.attribute( "geoPosX" ).toDouble(), annotationItemEl.attribute( "geoPosY" ).toDouble() ) ) );
      symbolItem.setAngle( -angle );

      QDomElement mapItemEl = doc.createElement( "MapItem" );
      mapItemEl.setAttribute( "name", "KadasSymbolItem" );
      mapItemEl.setAttribute( "crs", annotationItemEl.attribute( "mapGeoPosAuthID" ) );

      QJsonDocument jsonDoc;
      jsonDoc.setObject( symbolItem.serialize() );
      mapItemEl.appendChild( doc.createCDATASection( jsonDoc.toJson( QJsonDocument::Compact ) ) );

      newMapLayerEl.appendChild( mapItemEl );

      root.removeChild( svgItemEl );
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

      QDomElement mapItemEl = doc.createElement( "MapItem" );
      mapItemEl.setAttribute( "name", "KadasPictureItem" );
      mapItemEl.setAttribute( "crs", annotationItemEl.attribute( "mapGeoPosAuthID" ) );

      QJsonDocument jsonDoc;
      jsonDoc.setObject( pictureItem.serialize() );
      mapItemEl.appendChild( doc.createCDATASection( jsonDoc.toJson( QJsonDocument::Compact ) ) );

      newMapLayerEl.appendChild( mapItemEl );

      root.removeChild( imageItemEl );
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

      QDomElement mapItemEl = doc.createElement( "MapItem" );
      mapItemEl.setAttribute( "name", "KadasPinItem" );
      mapItemEl.setAttribute( "crs", annotationItemEl.attribute( "mapGeoPosAuthID" ) );
      mapItemEl.setAttribute( "editor", "KadasSymbolAttributesEditor" );

      QJsonDocument jsonDoc;
      jsonDoc.setObject( pinItem.serialize() );
      mapItemEl.appendChild( doc.createCDATASection( jsonDoc.toJson( QJsonDocument::Compact ) ) );

      newMapLayerEl.appendChild( mapItemEl );

      root.removeChild( pinItemEl );
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
    KadasMilxLayer layer( mapLayerEl.firstChildElement( "layername" ).text() );
    int dpi = kApp->mainWindow()->mapCanvas()->mapSettings().outputDpi();
    QString err;
    layer.importFromMilxly( mapLayerEl.firstChildElement( "MilXLayer" ), dpi, err );

    QDomElement newMapLayerEl = doc.createElement( "maplayer" );
    QgsReadWriteContext context;
    layer.writeXml( newMapLayerEl, doc, context );
    newMapLayerEl.appendChild( mapLayerEl.firstChildElement( "id" ).cloneNode() );
    newMapLayerEl.appendChild( mapLayerEl.firstChildElement( "layername" ).cloneNode() );

    projectLayersEl.replaceChild( newMapLayerEl, mapLayerEl );
  }

  // Changes to guide grid etc?
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
  foreach ( const QString &flag, flagsStr.split( ",", QString::SkipEmptyParts ) )
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
