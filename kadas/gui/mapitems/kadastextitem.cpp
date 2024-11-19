/***************************************************************************
    kadastextitem.cpp
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


#include <QAction>
#include <QMenu>

#include <qgis/qgsgeometryengine.h>
#include <qgis/qgslinestring.h>
#include <qgis/qgsmapsettings.h>
#include <qgis/qgspolygon.h>
#include <qgis/qgsproject.h>
#include <qgis/qgsrendercontext.h>
#include <qgis/qgstextdocument.h>
#include <qgis/qgstextrenderer.h>

#include "kadas/gui/mapitems/kadastextitem.h"


KadasTextItem::KadasTextItem( const QgsCoordinateReferenceSystem &crs )
  : KadasRectangleItemBase( crs )
{
}

void KadasTextItem::setText( const QString &text )
{
  mText = text;
  QFontMetrics metrics( mFont );
  if ( mFrameAutoResize )
  {
    state()->mSize.setWidth( metrics.horizontalAdvance( mText ) );
    state()->mSize.setHeight( metrics.height() );
  }
  update();
  emit propertyChanged();
}

void KadasTextItem::setAngle( double angle )
{
  state()->mAngle = angle;
  update();
}

void KadasTextItem::setFillColor( const QColor &c )
{
  mFillColor = c;
  update();
  emit propertyChanged();
}

void KadasTextItem::setOutlineColor( const QColor &c )
{
  mOutlineColor = c;
  update();
  emit propertyChanged();
}

void KadasTextItem::setFont( const QFont &font )
{
  mFont = font;
  QFontMetrics metrics( mFont );
  if ( mFrameAutoResize )
  {
    state()->mSize.setWidth( metrics.horizontalAdvance( mText ) );
    state()->mSize.setHeight( metrics.height() );
  }
  update();
  emit propertyChanged();
}

void KadasTextItem::setFrameAutoResize( bool frameAutoResize )
{
  mFrameAutoResize = frameAutoResize;
  emit propertyChanged();
  if ( mFrameAutoResize )
    setFont( mFont );
}

QImage KadasTextItem::symbolImage() const
{
  // TODO: remove, this seems unused
  QImage image( constState()->mSize, QImage::Format_ARGB32 );
  image.fill( Qt::transparent );
  QPainter painter( &image );

  painter.setBrush( QBrush( mFillColor ) );
  painter.setPen( QPen( mOutlineColor, mFont.pointSizeF() / 15., Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin ) );
  painter.setFont( mFont );
  painter.drawText( QRect( 0, 0, constState()->mSize.width(), constState()->mSize.height() ), mText );
  return image;
}

void KadasTextItem::renderPrivate( QgsRenderContext &context, const QPointF &center, const QRect &rect, double dpiScale ) const
{
  QFont font = mFont;
  font.setPointSizeF( font.pointSizeF() * outputDpiScale( context ) );
  QFontMetrics metrics( font );
  QPointF baseLineCenter = center + QPointF( 0, metrics.descent() );

  // no idea why this works, otherwise text scales up when edited
  // the rendex context is coming from KadasMapItem when edited while it comes from the QgsMapLayerRenderer otherwise
  double scale = 1.0;
  if ( context.painter()->device()->physicalDpiX() )
    scale = 1.0 / context.painter()->device()->physicalDpiX() * context.painter()->device()->logicalDpiX();

  QgsTextFormat format;
  format.setFont( font );
  format.setSize( font.pointSize() * scale );
  format.setColor( mFillColor );
  QgsTextBufferSettings bs;
  bs.setColor( mOutlineColor );
  bs.setSize( 1 );
  bs.setEnabled( true );
  format.setBuffer( bs );

  QgsTextRenderer::drawText( baseLineCenter, -constState()->mAngle, Qgis::TextHorizontalAlignment::Center, { mText }, context, format, false );
}

QString KadasTextItem::asKml( const QgsRenderContext &context, QuaZip *kmzZip ) const
{
  auto color2hex = []( const QColor &c ) { return QString( "%1%2%3%4" ).arg( c.alpha(), 2, 16, QChar( '0' ) ).arg( c.blue(), 2, 16, QChar( '0' ) ).arg( c.green(), 2, 16, QChar( '0' ) ).arg( c.red(), 2, 16, QChar( '0' ) ); };
  QgsPointXY pos = QgsCoordinateTransform( mCrs, QgsCoordinateReferenceSystem( "EPSG:4326" ), QgsProject::instance() ).transform( position() );

  QString outString;
  QTextStream outStream( &outString );
  outStream << "<Placemark>" << "\n";
  outStream << QString( "<name>%1</name>\n" ).arg( mText );
  outStream << "<Style>";
  outStream << QString( "<IconStyle><scale>0</scale></IconStyle><LabelStyle><color>%1</color><scale>%2</scale></LabelStyle></Style>" ).arg( color2hex( mFillColor ) ).arg( mFont.pointSizeF() / QFont().pointSizeF() );
  outStream << QString( "<Point><coordinates>%1,%2</coordinates></Point>" ).arg( QString::number( pos.x(), 'f', 10 ) ).arg( QString::number( pos.y(), 'f', 10 ) );
  outStream << "</Placemark>" << "\n";
  outStream.flush();
  return outString;
}

void KadasTextItem::editPrivate( const KadasMapPos &newPoint, const QgsMapSettings &mapSettings )
{
  QSize oldSize = state()->mSize;

  double scale = mapSettings.mapUnitsPerPixel() * mSymbolScale;
  KadasMapPos mapPos = toMapPos( constState()->mPos, mapSettings );
  KadasMapPos frameCenter( mapPos.x() + constState()->mOffsetX * scale, mapPos.y() + constState()->mOffsetY * scale );

  QgsVector halfSize = ( mapSettings.mapToPixel().transform( newPoint ) - mapSettings.mapToPixel().transform( frameCenter ) ) / mSymbolScale;

  double ratio = std::min( 2 * qAbs( halfSize.x() / oldSize.width() ), 2 * qAbs( halfSize.y() / oldSize.height() ) );

  if ( mFrameAutoResize )
  {
    QFont font = mFont;
    font.setPointSizeF( font.pointSizeF() * ratio );
    setFont( font );
  }
  else
  {
    state()->mSize.setWidth( 2 * qAbs( halfSize.x() ) );
    state()->mSize.setHeight( 2 * qAbs( halfSize.y() ) );
    update();
  }
}

void KadasTextItem::populateContextMenuPrivate( QMenu *menu, const EditContext &context, const KadasMapPos &clickPos, const QgsMapSettings &mapSettings )
{
  if ( frameVisible() )
  {
    QAction *lockedAction = menu->addAction( tr( "Auto resize frame" ), [this]( bool autoResize ) { setFrameAutoResize( autoResize ); } );
    lockedAction->setCheckable( true );
    lockedAction->setChecked( mFrameAutoResize );
  }
}
