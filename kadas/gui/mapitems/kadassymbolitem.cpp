/***************************************************************************
    kadassymbolitem.cpp
    --------------------
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

#include <QSvgRenderer>
#include <QImageReader>

#include <quazip5/quazipfile.h>

#include <qgis/qgsgeometryengine.h>
#include <qgis/qgslinestring.h>
#include <qgis/qgsmapsettings.h>
#include <qgis/qgspolygon.h>
#include <qgis/qgsproject.h>

#include <kadas/gui/mapitems/kadassymbolitem.h>


KADAS_REGISTER_MAP_ITEM( KadasSymbolItem, []( const QgsCoordinateReferenceSystem &crs )  { return new KadasSymbolItem( crs ); } );

KadasSymbolItem::KadasSymbolItem( const QgsCoordinateReferenceSystem &crs, QObject *parent )
  : KadasAnchoredItem( crs, parent )
{
  clear();
}

void KadasSymbolItem::setup( const QString &path, double anchorX, double anchorY )
{
  mAnchorX = anchorX;
  mAnchorY = anchorY;
  setFilePath( path );
}

void KadasSymbolItem::setFilePath( const QString &path )
{
  mFilePath = path;
  QImageReader reader( path );
  mScalable = reader.format() == "svg";
  state()->size = reader.size();
  reader.setBackgroundColor( Qt::transparent );
  mImage = reader.read().convertToFormat( QImage::Format_ARGB32 );

  update();
}

void KadasSymbolItem::render( QgsRenderContext &context ) const
{
  if ( constState()->drawStatus == State::Empty )
  {
    return;
  }

  QgsPoint pos = QgsPoint( constState()->pos );
  pos.transform( context.coordinateTransform() );
  pos.transform( context.mapToPixel().transform() );

  double scale = 1.0; // TODO
  context.painter()->scale( scale, scale );
  context.painter()->translate( pos.x(), pos.y() );
  context.painter()->rotate( -constState()->angle );
  context.painter()->translate( - mAnchorX * constState()->size.width(), - mAnchorY * constState()->size.height() );
  if ( mScalable )
  {
    QSvgRenderer svgRenderer( mFilePath );
    QSize renderSize = svgRenderer.viewBox().size() * scale;
    svgRenderer.render( context.painter(), QRectF( 0, 0, renderSize.width(), renderSize.height() ) );

  }
  else
  {
    context.painter()->drawImage( 0, 0, mImage );
  }
}

QString KadasSymbolItem::asKml( const QgsRenderContext &context, QuaZip *kmzZip ) const
{
  if ( !kmzZip )
  {
    // Can only export to KMZ
    return "";
  }

  QString fileName = QUuid::createUuid().toString();
  fileName = fileName.mid( 1, fileName.length() - 2 ) + ".png";
  QuaZipFile outputFile( kmzZip );
  QuaZipNewInfo info( fileName );
  info.setPermissions( QFile::ReadOwner | QFile::ReadUser | QFile::ReadGroup | QFile::ReadOther );
  if ( !outputFile.open( QIODevice::WriteOnly, info ) || !mImage.save( &outputFile, "PNG" ) )
  {
    return "";
  }

  double hotSpotX = mAnchorX * constState()->size.width();
  double hotSpotY = mAnchorY * constState()->size.height();
  QgsPointXY pos = QgsCoordinateTransform( mCrs, QgsCoordinateReferenceSystem( "EPSG:4326" ), QgsProject::instance() ).transform( position() );

  QString outString;
  QTextStream outStream( &outString );

  QString id = QUuid::createUuid().toString();
  id = id.mid( 1, id.length() - 2 );
  outStream << "<StyleMap id=\"" << id << "\">" << "\n";
  outStream << "  <Pair>" << "\n";
  outStream << "    <key>normal</key>" << "\n";
  outStream << "    <Style>" << "\n";
  outStream << "      <IconStyle>" << "\n";
  outStream << "        <scale>1.0</scale>" << "\n";
  outStream << "        <Icon><href>" << fileName << "</href></Icon>" << "\n";
  outStream << "        <hotSpot x=\"" << hotSpotX << "\" y=\"" << hotSpotY << "\" xunits=\"insetPixels\" yunits=\"insetPixels\" />" << "\n";
  outStream << "      </IconStyle>" << "\n";
  outStream << "    </Style>" << "\n";
  outStream << "  </Pair>" << "\n";
  outStream << "  <Pair>" << "\n";
  outStream << "    <key>highlight</key>" << "\n";
  outStream << "    <Style>" << "\n";
  outStream << "      <IconStyle>" << "\n";
  outStream << "        <scale>1.0</scale>" << "\n";
  outStream << "        <Icon><href>" << fileName << "</href></Icon>" << "\n";
  outStream << "        <hotSpot x=\"" << hotSpotX << "\" y=\"" << hotSpotY << "\" xunits=\"insetPixels\" yunits=\"insetPixels\" />" << "\n";
  outStream << "      </IconStyle>" << "\n";
  outStream << "    </Style>" << "\n";
  outStream << "  </Pair>" << "\n";
  outStream << "</StyleMap>" << "\n";
  outStream << "<Placemark>" << "\n";
  outStream << "  <name>" << itemName() << "</name>" << "\n";
  outStream << "  <styleUrl>#" << id << "</styleUrl>" << "\n";
  outStream << "  <Point>" << "\n";
  outStream << "    <coordinates>" << QString::number( pos.x(), 'f', 10 ) << "," << QString::number( pos.y(), 'f', 10 ) << ",0</coordinates>" << "\n";
  outStream << "  </Point>" << "\n";
  outStream << "</Placemark>" << "\n";
  outStream.flush();

  return outString;
}

KadasPinItem::KadasPinItem( const QgsCoordinateReferenceSystem &crs, QObject *parent )
  : KadasSymbolItem( crs, parent )
{
  setup( ":/kadas/icons/pin_red", 0.5, 1.0 );
}
