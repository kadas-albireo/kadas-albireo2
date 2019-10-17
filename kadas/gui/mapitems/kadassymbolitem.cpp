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

#include <qgis/qgsgeometryengine.h>
#include <qgis/qgslinestring.h>
#include <qgis/qgsmapsettings.h>
#include <qgis/qgspolygon.h>
#include <qgis/qgsproject.h>

#include <kadas/gui/mapitems/kadassymbolitem.h>


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
