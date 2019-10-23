/***************************************************************************
    kdaskmlimport.cpp
    -----------------
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


#include <QDomDocument>
#include <QFileInfo>
#include <QUuid>

#include <quazip5/quazipfile.h>

#include <qgis/qgslinestring.h>
#include <qgis/qgslogger.h>
#include <qgis/qgspolygon.h>
#include <qgis/qgsproject.h>
#include <qgis/qgsrasterlayer.h>
#include <qgis/qgssymbollayerutils.h>

#include <kadas/core/kadastemporaryfile.h>
#include <kadas/gui/kadasitemlayer.h>
#include <kadas/gui/mapitems/kadaslineitem.h>
#include <kadas/gui/mapitems/kadassymbolitem.h>
#include <kadas/gui/mapitems/kadaspointitem.h>
#include <kadas/gui/mapitems/kadaspolygonitem.h>
#include <kadas/gui/mapitems/kadastextitem.h>
#include <kadas/gui/mapitemeditors/kadasredliningitemeditor.h>
#include <kadas/gui/mapitemeditors/kadasredliningtexteditor.h>
#include <kadas/app/kadasapplication.h>
#include <kadas/app/kml/kadaskmlimport.h>


bool KadasKMLImport::importFile( const QString &filename, QString &errMsg )
{
  if ( filename.endsWith( ".kmz", Qt::CaseInsensitive ) )
  {
    QuaZip quaZip( filename );
    if ( !quaZip.open( QuaZip::mdUnzip ) )
    {
      errMsg = tr( "Unable to open %1." ).arg( QFileInfo( filename ).fileName() );
      return false;
    }
    // Search for kml file to open
    QStringList kmzFileList = quaZip.getFileNameList();
    int mainKmlIndex = kmzFileList.indexOf( QRegExp( "[^/]+.kml", Qt::CaseInsensitive ) );
    if ( mainKmlIndex == -1 || !quaZip.setCurrentFile( kmzFileList[mainKmlIndex] ) )
    {
      errMsg = tr( "Corrupt KMZ file." );
      return false;
    }
    QDomDocument doc;
    QuaZipFile file( &quaZip );
    if ( !file.open( QIODevice::ReadOnly ) || !doc.setContent( &file ) )
    {
      errMsg = tr( "Corrupt KMZ file." );
      return false;
    }
    file.close();
    return importDocument( QFileInfo( filename ).fileName(), doc, errMsg, &quaZip );
  }
  else if ( filename.endsWith( ".kml", Qt::CaseInsensitive ) )
  {
    QFile file( filename );
    QDomDocument doc;
    if ( !file.open( QIODevice::ReadOnly ) || !doc.setContent( &file ) )
    {
      errMsg = tr( "Unable to open %1." ).arg( QFileInfo( filename ).fileName() );
      return false;
    }
    return importDocument( QFileInfo( filename ).fileName(), doc, errMsg );
  }
  return false;
}

bool KadasKMLImport::importDocument( const QString &filename, const QDomDocument &doc, QString &errMsg, QuaZip *zip )
{
  QDomElement documentEl = doc.firstChildElement( "kml" ).firstChildElement( "Document" );
  if ( documentEl.isNull() )
  {
    errMsg = tr( "Corrupt KMZ file." );
    return false;
  }

  QgsCoordinateReferenceSystem crsWgs84( "EPSG:4326" );
  KadasItemLayer *itemLayer = kApp->getOrCreateItemLayer( filename );
  QgsCoordinateTransform itemCrst( crsWgs84, itemLayer->crs(), QgsProject::instance()->transformContext() );

  // Styles / StyleMaps with id
  QMap<QString, StyleData> styleMap;
  QDomNodeList styleEls = documentEl.elementsByTagName( "Style" );
  for ( int iStyle = 0, nStyles = styleEls.size(); iStyle < nStyles; ++iStyle )
  {
    QDomElement styleEl = styleEls.at( iStyle ).toElement();
    if ( !styleEl.attribute( "id" ).isEmpty() )
    {
      styleMap.insert( QString( "#%1" ).arg( styleEl.attribute( "id" ) ), parseStyle( styleEl, zip ) );
    }
  }
  QDomNodeList styleMapEls = documentEl.elementsByTagName( "StyleMap" );
  for ( int iStyleMap = 0, nStyleMaps = styleMapEls.size(); iStyleMap < nStyleMaps; ++iStyleMap )
  {
    QDomElement styleMapEl = styleMapEls.at( iStyleMap ).toElement();
    if ( !styleMapEl.attribute( "id" ).isEmpty() )
    {
      QString id = QString( "#%1" ).arg( styleMapEl.attribute( "id" ) );
      // Just pick the first item of the StyleMap
      QDomElement pairEl = styleMapEl.firstChildElement( "Pair" );
      QDomElement styleEl = pairEl.firstChildElement( "Style" );
      QDomElement styleUrlEl = pairEl.firstChildElement( "styleUrl" );
      if ( !styleEl.isNull() )
      {
        styleMap.insert( id, parseStyle( styleEl, zip ) );
      }
      else if ( !styleUrlEl.isNull() )
      {
        styleMap.insert( id, styleMap.value( styleUrlEl.text() ) );
      }
    }
  }


  // Placemarks
  QDomNodeList placemarkEls = documentEl.elementsByTagName( "Placemark" );
  for ( int iPlacemark = 0, nPlacemarks = placemarkEls.size(); iPlacemark < nPlacemarks; ++iPlacemark )
  {
    QDomElement placemarkEl = placemarkEls.at( iPlacemark ).toElement();
    QString name = placemarkEl.firstChildElement( "name" ).text();

    // Geometry
    QList<QgsAbstractGeometry *> geoms = parseGeometries( placemarkEl );

    if ( geoms.isEmpty() )
    {
      // Placemark without geometry
      QgsDebugMsg( "Could not parse placemark geometry" );
      continue;
    }

    // Style
    QDomElement styleEl = placemarkEl.firstChildElement( "Style" );
    QDomElement styleUrlEl = placemarkEl.firstChildElement( "styleUrl" );
    StyleData style;
    if ( !styleEl.isNull() )
    {
      style = parseStyle( styleEl, zip );
    }
    else if ( !styleUrlEl.isNull() )
    {
      style = styleMap.value( styleUrlEl.text() );
    }

    QMap<QString, QString> attributes = parseExtendedData( placemarkEl );

    // If there is an icon and the geometry is a point, add as symbol item, otherwise as redlining symbol
    if ( geoms.size() == 1 && !style.icon.isEmpty() && dynamic_cast<QgsPoint *>( geoms.front() ) )
    {
      QgsPointXY pos = itemCrst.transform( *static_cast<QgsPoint *>( geoms.front() ) );
      KadasSymbolItem *item = new KadasSymbolItem( itemLayer->crs() );
      item->setFilePath( style.icon );
      item->setAnchorX( style.hotSpot.x() / item->constState()->size.width() );
      item->setAnchorY( style.hotSpot.y() / item->constState()->size.height() );
      item->setPosition( KadasItemPos::fromPoint( pos ) );
      itemLayer->addItem( item );
      delete geoms.front();
    }
    else
    {
      // TODO?
      QStringList attributesList;
      for ( auto it = attributes.begin(), itEnd = attributes.end(); it != itEnd; ++it )
      {
        attributesList.append( it.key().toHtmlEscaped() + "=" + it.value().replace( QRegularExpression( "^\\s*(.*[^\\s])\\s*$", QRegularExpression::MultilineOption ), "\\1" ).toHtmlEscaped() );
      }
      QString attributesStr = attributesList.join( "&" );

      for ( QgsAbstractGeometry *geom : geoms )
      {
        geom->transform( itemCrst );

        QgsWkbTypes::Type type = QgsWkbTypes::singleType( QgsWkbTypes::flatType( geom->wkbType() ) );
        if ( dynamic_cast<QgsPoint *>( geom ) && style.isLabel )
        {
          QgsPointXY pos = *static_cast<QgsPoint *>( geom );
          KadasTextItem *item = new KadasTextItem( itemLayer->crs() );
          item->setEditor( "KadasRedliningTextEditor" );
          item->setText( name );
          item->setFillColor( style.labelColor );
          QFont font = item->font();
          font.setPointSizeF( font.pointSizeF() * style.labelScale );
          item->setFont( font );
          item->setPosition( KadasItemPos::fromPoint( pos ) );
          itemLayer->addItem( item );
        }
        else if ( dynamic_cast<QgsPoint *>( geom ) )
        {
          KadasPointItem *item = new KadasPointItem( itemLayer->crs() );
          item->setEditor( "KadasRedliningItemEditor" );
          item->addPartFromGeometry( *geom );
          item->setIconSize( 10 + 2 * style.outlineSize );
          item->setIconOutline( QPen( style.outlineColor, style.outlineSize ) );
          item->setIconFill( QBrush( style.fillColor ) );
          kApp->getOrCreateItemLayer( filename )->addItem( item );
        }
        else if ( dynamic_cast<QgsLineString *>( geom ) )
        {
          KadasLineItem *item = new KadasLineItem( itemLayer->crs() );
          item->setEditor( "KadasRedliningItemEditor" );
          item->addPartFromGeometry( *geom );
          item->setOutline( QPen( style.outlineColor, style.outlineSize ) );
          itemLayer->addItem( item );
        }
        else if ( dynamic_cast<QgsPolygon *>( geom ) )
        {
          KadasPolygonItem *item = new KadasPolygonItem( itemLayer->crs() );
          item->setEditor( "KadasRedliningItemEditor" );
          item->addPartFromGeometry( *geom );
          item->setOutline( QPen( style.outlineColor, style.outlineSize ) );
          item->setFill( QBrush( style.fillColor ) );
          kApp->getOrCreateItemLayer( filename )->addItem( item );
        }
      }
      qDeleteAll( geoms );
    }
  }

  // Ground overlays: group by name of parent folder, if possible
  if ( zip )
  {
    QMap<QString, OverlayData> overlays;
    QDomNodeList groundOverlayEls = documentEl.elementsByTagName( "GroundOverlay" );
    for ( int iOverlay = 0, nOverlays = groundOverlayEls.size(); iOverlay < nOverlays; ++iOverlay )
    {
      QDomElement groundOverlayEl = groundOverlayEls.at( iOverlay ).toElement();
      QDomElement bboxEl = groundOverlayEl.firstChildElement( "LatLonBox" ).toElement();
      QString name = groundOverlayEl.firstChildElement( "name" ).text();
      // If tile contained in folder, group by folder
      if ( groundOverlayEl.parentNode().nodeName() == "Folder" )
      {
        name = groundOverlayEl.parentNode().firstChildElement( "name" ).text();
      }
      TileData tile;
      tile.iconHref = groundOverlayEl.firstChildElement( "Icon" ).firstChildElement( "href" ).text();
      tile.bbox.setXMinimum( bboxEl.firstChildElement( "west" ).text().toDouble() );
      tile.bbox.setXMaximum( bboxEl.firstChildElement( "east" ).text().toDouble() );
      tile.bbox.setYMinimum( bboxEl.firstChildElement( "south" ).text().toDouble() );
      tile.bbox.setYMaximum( bboxEl.firstChildElement( "north" ).text().toDouble() );
      overlays[name].tiles.append( tile );
      if ( overlays[name].bbox.isEmpty() )
      {
        overlays[name].bbox = tile.bbox;
      }
      else
      {
        overlays[name].bbox.combineExtentWith( tile.bbox );
      }
    }

    // Build VRTs for each overlay group
    for ( auto it = overlays.begin(), itEnd = overlays.end(); it != itEnd; ++it )
    {
      buildVSIVRT( it.key(), it.value(), zip );
    }
  }

  return true;
}

void KadasKMLImport::buildVSIVRT( const QString &name, OverlayData &overlayData, QuaZip *kmzZip ) const
{
  if ( overlayData.tiles.empty() )
  {
    return;
  }

  // Prepare vsi output
  QString safename = name;
  safename.replace( QRegExp( "[<>:\"/\\\\|?*]" ), "_" ); // Replace invalid path chars
  QString vsifilename = KadasTemporaryFile::createNewFile( QString( "%1.zip" ).arg( safename ) );
  QuaZip vsiZip( vsifilename );
  if ( !vsiZip.open( QuaZip::mdCreate ) )
  {
    return;
  }
  // Copy tile images to vsi, determine tile sizes
  for ( TileData &tile : overlayData.tiles )
  {
    if ( !kmzZip->setCurrentFile( tile.iconHref ) )
    {
      continue;
    }
    QuaZipFile kmzTileFile( kmzZip );
    QuaZipFile vsiTileFile( &vsiZip );
    QuaZipNewInfo tileInfo( QFileInfo( tile.iconHref ).completeBaseName() + ".png" );
    tileInfo.setPermissions( QFile::ReadOwner | QFile::ReadUser | QFile::ReadGroup | QFile::ReadOther );
    if ( !kmzTileFile.open( QIODevice::ReadOnly ) || !vsiTileFile.open( QIODevice::WriteOnly, tileInfo ) )
    {
      continue;
    }
    QImage tileImage = QImage::fromData( kmzTileFile.readAll() );
    tile.size = tileImage.size();
    tileImage.save( &vsiTileFile, "PNG" );
    vsiTileFile.close();
  }

  // Get total size by assuming all tiles have same resolution
  const TileData &firstTile = overlayData.tiles.front();
  QSize totSize(
    qRound( firstTile.size.width() / firstTile.bbox.width() * overlayData.bbox.width() ),
    qRound( firstTile.size.height() / firstTile.bbox.height() * overlayData.bbox.height() )
  );

  // Write vrt
  QString vrtString;
  QTextStream vrtStream( &vrtString, QIODevice::WriteOnly );
  vrtStream << "<VRTDataset rasterXSize=\"" << totSize.width() << "\" rasterYSize=\"" << totSize.height() << "\">" << endl;
  vrtStream << " <SRS>" << QgsCoordinateReferenceSystem( "EPSG:4326" ).toProj4().toHtmlEscaped() << "</SRS>" << endl;
  vrtStream << " <GeoTransform>" << endl;
  vrtStream << "  " << overlayData.bbox.xMinimum() << "," << ( overlayData.bbox.width() / totSize.width() ) << ", 0," << endl;
  vrtStream << "  " << overlayData.bbox.yMaximum() << ", 0," << ( -overlayData.bbox.height() / totSize.height() ) << endl;
  vrtStream << " </GeoTransform>" << endl;
  for ( const QPair<int, QString> &band : QList<QPair<int, QString>> {qMakePair( 1, QString( "Red" ) ), qMakePair( 2, QString( "Green" ) ), qMakePair( 3, QString( "Blue" ) ), qMakePair( 4, QString( "Alpha" ) )} )
  {
    vrtStream << " <VRTRasterBand dataType=\"Byte\" band=\"" << band.first << "\">" << endl;
    vrtStream << "  <ColorInterp>" << band.second << "</ColorInterp>" << endl;
    for ( const TileData &tile : overlayData.tiles )
    {
      int i = qRound( ( tile.bbox.xMinimum() - overlayData.bbox.xMinimum() ) / overlayData.bbox.width() * totSize.width() );
      int j = qRound( ( overlayData.bbox.yMaximum() - tile.bbox.yMaximum() ) / overlayData.bbox.height() * totSize.height() );
      vrtStream << "  <SimpleSource>" << endl;
      vrtStream << "   <SourceFilename relativeToVRT=\"1\">" << tile.iconHref << "</SourceFilename>" << endl;
      vrtStream << "   <SourceBand>" << band.first << "</SourceBand>" << endl;
      vrtStream << "   <SourceProperties RasterXSize=\"" << tile.size.width() << "\" RasterYSize=\"" << tile.size.height() << "\" DataType=\"Byte\" BlockXSize=\"" << tile.size.width() << "\" BlockYSize=\"1\" />" << endl;
      vrtStream << "   <SrcRect xOff=\"0\" yOff=\"0\" xSize=\"" << tile.size.width() << "\" ySize=\"" << tile.size.height() << "\" />" << endl;
      vrtStream << "   <DstRect xOff=\"" << i << "\" yOff=\"" << j << "\" xSize=\"" << tile.size.width() << "\" ySize=\"" << tile.size.height() << "\" />" << endl;
      vrtStream << "  </SimpleSource>" << endl;
    }
    vrtStream << " </VRTRasterBand>" << endl;
  }
  vrtStream << "</VRTDataset>" << endl;
  vrtStream.flush();

  QuaZipFile vsiVrtFile( &vsiZip );
  QuaZipNewInfo vrtInfo( "dataset.vrt" );
  vrtInfo.setPermissions( QFile::ReadOwner | QFile::ReadUser | QFile::ReadGroup | QFile::ReadOther );
  if ( !vsiVrtFile.open( QIODevice::WriteOnly, vrtInfo ) )
  {
    return;
  }
  vsiVrtFile.write( vrtString.toLocal8Bit() );
  vsiVrtFile.close();
  vsiZip.close();

  QgsRasterLayer *rasterLayer = new QgsRasterLayer( QString( "/vsizip/%1/dataset.vrt" ).arg( vsifilename ), name, "gdal" );
  QgsProject::instance()->addMapLayer( rasterLayer );
}

QVector<QgsPoint> KadasKMLImport::parseCoordinates( const QDomElement &geomEl ) const
{
  QStringList coordinates = geomEl.firstChildElement( "coordinates" ).text().split( QRegExp( "\\s+" ), QString::SkipEmptyParts );
  QVector<QgsPoint> points;
  for ( int i = 0, n = coordinates.size(); i < n; ++i )
  {
    QStringList coordinate = coordinates[i].split( "," );
    if ( coordinate.size() >= 3 )
    {
      QgsPoint p( QgsWkbTypes::PointZ );
      p.setX( coordinate[0].toDouble() );
      p.setY( coordinate[1].toDouble() );
      p.setZ( coordinate[2].toDouble() );
      points.append( p );
    }
    else if ( coordinate.size() == 2 )
    {
      QgsPoint p( QgsWkbTypes::Point );
      p.setX( coordinate[0].toDouble() );
      p.setY( coordinate[1].toDouble() );
      points.append( p );
    }
  }
  return points;
}

KadasKMLImport::StyleData KadasKMLImport::parseStyle( const QDomElement &styleEl, QuaZip *zip ) const
{
  StyleData style;

  QDomElement lineStyleEl = styleEl.firstChildElement( "LineStyle" );
  QDomElement lineWidthEl = lineStyleEl.firstChildElement( "width" );
  style.outlineSize = lineWidthEl.isNull() ? 1 : lineWidthEl.text().toDouble();

  QDomElement lineColorEl = lineStyleEl.firstChildElement( "color" );
  style.outlineColor = lineColorEl.isNull() ? QColor( Qt::black ) : parseColor( lineColorEl.text() );

  QDomElement polyStyleEl = styleEl.firstChildElement( "PolyStyle" );
  QDomElement fillColorEl = polyStyleEl.firstChildElement( "color" );
  style.fillColor = fillColorEl.isNull() ? QColor( Qt::white ) : parseColor( fillColorEl.text() );

  QDomElement labelStyleEl = styleEl.firstChildElement( "LabelStyle" );
  QDomElement labelColorEl = labelStyleEl.firstChildElement( "color" );
  QDomElement labelScaleEl = labelStyleEl.firstChildElement( "scale" );
  style.isLabel = !labelScaleEl.isNull();
  style.labelColor = labelColorEl.isNull() ? QColor( Qt::black ) : parseColor( labelColorEl.text() );
  style.labelScale = labelScaleEl.isNull() ? 1. : labelScaleEl.text().toDouble();

  if ( polyStyleEl.firstChildElement( "fill" ).text() == "0" )
  {
    style.fillColor = QColor( Qt::transparent );
  }
  if ( polyStyleEl.firstChildElement( "outline" ).text() == "0" )
  {
    style.outlineColor = QColor( Qt::transparent );
  }

  QDomElement iconHRefEl = styleEl.firstChildElement( "IconStyle" ).firstChildElement( "Icon" ).firstChildElement( "href" );
  if ( !iconHRefEl.isNull() )
  {
    QString filename = iconHRefEl.text();
    // Only local files in KMZ are supported (also for security reasons)
    if ( zip && zip->setCurrentFile( filename ) )
    {
      QuaZipFile file( zip );
      if ( file.open( QIODevice::ReadOnly ) )
      {
        QImage icon = QImage::fromData( file.readAll() );
        style.icon = KadasTemporaryFile::createNewFile( "kml_import.png" );
        icon.save( style.icon );

        QDomElement hotSpotEl = styleEl.firstChildElement( "IconStyle" ).firstChildElement( "hotSpot" );
        if ( !hotSpotEl.isNull() )
        {
          double x = hotSpotEl.attribute( "x" ).toDouble();
          double y = hotSpotEl.attribute( "y" ).toDouble();
          if ( hotSpotEl.attribute( "xunits" ) == "fraction" )
          {
            x *= icon.width();
          }
          if ( hotSpotEl.attribute( "yunits" ) == "fraction" )
          {
            y *= icon.height();
          }
          style.hotSpot = QPointF( x, y );
        }
      }
    }
  }
  return style;
}

QList<QgsAbstractGeometry *> KadasKMLImport::parseGeometries( const QDomElement &containerEl )
{
  QList<QgsAbstractGeometry *> geoms;

  QDomElement pointEl = containerEl.firstChildElement( "Point" );
  if ( !pointEl.isNull() )
  {
    QVector<QgsPoint> points = parseCoordinates( pointEl );
    if ( !points.isEmpty() )
    {
      geoms.append( points[0].clone() );
    }
  }

  QDomElement lineStringEl = containerEl.firstChildElement( "LineString" );
  if ( !lineStringEl.isNull() )
  {
    QgsLineString *line = new QgsLineString();
    line->setPoints( parseCoordinates( lineStringEl ) );
    geoms.append( line );
  }

  QDomElement polygonEl = containerEl.firstChildElement( "Polygon" );
  if ( !polygonEl.isNull() )
  {
    QDomElement outerRingEl = polygonEl.firstChildElement( "outerBoundaryIs" ).firstChildElement( "LinearRing" );
    QgsLineString *exterior = new QgsLineString();
    exterior->setPoints( parseCoordinates( outerRingEl ) );
    QgsPolygon *poly = new QgsPolygon();
    poly->setExteriorRing( exterior );

    QDomNodeList innerBoundaryEls = polygonEl.elementsByTagName( "innerBoundaryIs" );
    for ( int iRing = 0, nRings = innerBoundaryEls.size(); iRing < nRings; ++iRing )
    {
      QDomElement innerRingEl = innerBoundaryEls.at( iRing ).toElement().firstChildElement( "LinearRing" );
      QgsLineString *interior = new QgsLineString();
      interior->setPoints( parseCoordinates( innerRingEl ) );
      poly->addInteriorRing( interior );
    }
    geoms.append( poly );
  }

  QDomElement multiGeometryEl = containerEl.firstChildElement( "MultiGeometry" );
  if ( !multiGeometryEl.isNull() )
  {
    geoms.append( parseGeometries( multiGeometryEl ) );
  }
  return geoms;
}

QColor KadasKMLImport::parseColor( const QString &abgr ) const
{
  if ( abgr.length() < 8 )
  {
    return Qt::black;
  }
  int a = abgr.mid( 0, 2 ).toInt( nullptr, 16 );
  int b = abgr.mid( 2, 2 ).toInt( nullptr, 16 );
  int g = abgr.mid( 4, 2 ).toInt( nullptr, 16 );
  int r = abgr.mid( 6, 2 ).toInt( nullptr, 16 );
  return QColor( r, g, b, a );
}

QMap<QString, QString> KadasKMLImport::parseExtendedData( const QDomElement &placemarkEl )
{
  QMap<QString, QString> attributes;
  QDomNodeList simpleDataEls = placemarkEl.elementsByTagName( "SimpleData" );
  for ( int iSimpleData = 0, nSimpleData = simpleDataEls.size(); iSimpleData < nSimpleData; ++iSimpleData )
  {
    QDomElement simpleDataEl = simpleDataEls.at( iSimpleData ).toElement();
    attributes.insert( simpleDataEl.attribute( "name" ), simpleDataEl.text() );
  }
  return attributes;
}
