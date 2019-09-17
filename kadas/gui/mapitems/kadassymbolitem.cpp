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

void KadasSymbolItem::setFilePath( const QString &path, double anchorX, double anchorY )
{
  mFilePath = path;
  mAnchorX = anchorX;
  mAnchorY = anchorY;
  QSvgRenderer renderer( mFilePath );
  state()->size = renderer.viewBox().size();
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

  QSvgRenderer svgRenderer( mFilePath );

  //keep width/height ratio of svg
  QRect viewBox = svgRenderer.viewBox();
  if ( viewBox.isValid() )
  {
    double scale = 1.0; // TODO
    QSize frameSize = viewBox.size() * scale;
    double widthRatio = frameSize.width() / viewBox.width();
    double heightRatio = frameSize.height() / viewBox.height();
    double renderWidth = 0;
    double renderHeight = 0;
    if ( widthRatio <= heightRatio )
    {
      renderWidth = frameSize.width();
      renderHeight = viewBox.height() * frameSize.width() / viewBox.width();
    }
    else
    {
      renderHeight = frameSize.height();
      renderWidth = viewBox.width() * frameSize.height() / viewBox.height();
    }
    context.painter()->scale( scale, scale );
    context.painter()->translate( pos.x(), pos.y() );
    context.painter()->rotate( -constState()->angle );
    context.painter()->translate( - mAnchorX * constState()->size.width(), - mAnchorY * constState()->size.height() );
    svgRenderer.render( context.painter(), QRectF( 0, 0, renderWidth, renderHeight ) );
  }
}
