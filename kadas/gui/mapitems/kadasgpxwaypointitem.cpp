/***************************************************************************
    kadasgpxwaypointitem.cpp
    ------------------------
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

#include <QFontMetrics>
#include <QPainter>
#include <QPainterPath>

#include <qgis/qgsproject.h>
#include <qgis/qgsrendercontext.h>
#include <qgis/qgstextrenderer.h>
#include <qgis/qgssymbollayerutils.h>

#include "kadas/gui/mapitems/kadasgpxwaypointitem.h"


KadasGpxWaypointItem::KadasGpxWaypointItem()
  : KadasPointItem( QgsCoordinateReferenceSystem( "EPSG:4326" ), Qgis::MarkerShape::Circle )
{
  setEditor( KadasMapItemEditor::GPX_WAYPOINT );
  mLabelFont.setPointSize( 10 );
  mLabelFont.setBold( true );

  // Default style
  int size = 2;
  QColor color = Qt::yellow;

  setColor( color );
  setStrokeColor( color );
  setStrokeWidth( size );

  setIconSize( 10 + 2 * size );
}

QString KadasGpxWaypointItem::exportName() const
{
  if ( name().isEmpty() )
    return itemName();

  return name();
}

void KadasGpxWaypointItem::setName( const QString &name )
{
  mName = name;
  QFontMetrics fm( mLabelFont );
  mLabelSize = fm.size( 0, name );
  update();
  emit propertyChanged();
}

void KadasGpxWaypointItem::setLabelFont( const QFont &labelFont )
{
  mLabelFont = labelFont;
  emit propertyChanged();
}

void KadasGpxWaypointItem::setLabelColor( const QColor &labelColor )
{
  mLabelColor = labelColor;
  emit propertyChanged();
}

KadasMapItem::Margin KadasGpxWaypointItem::margin() const
{
  Margin m = KadasPointItem::margin();
  if ( !mName.isEmpty() )
  {
    m.right = std::max( m.right, mLabelSize.width() + iconSize() / 2 + 1 );
    m.top = std::max( m.top, mLabelSize.height() + iconSize() / 2 + 1 );
  }
  return m;
}

void KadasGpxWaypointItem::render( QgsRenderContext &context ) const
{
  KadasPointItem::render( context );

  // Draw name label
  if ( !mName.isEmpty() && !constState()->points.isEmpty() )
  {
    QPointF pos = context.mapToPixel().transform( context.coordinateTransform().transform( constState()->points.front() ) ).toQPointF();
    pos += QPointF( 0.5 * iconSize(), -0.5 * iconSize() );
    QColor bufferColor = ( 0.2126 * color().red() + 0.7152 * color().green() + 0.0722 * color().blue() ) > 128 ? Qt::black : Qt::white;

    QFont font = mLabelFont;
    font.setPointSizeF( font.pointSizeF() * outputDpiScale( context ) );
    QFontMetrics metrics( font );
    pos += QPointF( metrics.width( mName ) / 2.0, 0.0 );

    double scale = getTextRenderScale( context );

    QgsTextFormat format;
    format.setFont( font );
    format.setSize( font.pointSize() * scale );
    if ( mLabelColor.isValid() )
      format.setColor( mLabelColor );
    else
      format.setColor( color() );
    QgsTextBufferSettings bs;
    bs.setColor( bufferColor );
    bs.setSize( 1 );
    bs.setEnabled( true );
    format.setBuffer( bs );

    QgsTextRenderer::drawText( pos, 0.0, Qgis::TextHorizontalAlignment::Center, { mName }, context, format, false );
  }
}

QString KadasGpxWaypointItem::asKml( const QgsRenderContext &context, QuaZip *kmzZip ) const
{
  if ( annotationItem()->geometry().isEmpty() )
  {
    return QString();
  }

  auto color2hex = []( const QColor &c ) { return QString( "%1%2%3%4" ).arg( c.alpha(), 2, 16, QChar( '0' ) ).arg( c.blue(), 2, 16, QChar( '0' ) ).arg( c.green(), 2, 16, QChar( '0' ) ).arg( c.red(), 2, 16, QChar( '0' ) ); };

  QString outString;
  QTextStream outStream( &outString );
  outStream << "<Placemark>\n";
  outStream << QString( "<name>%1</name>\n" ).arg( exportName() );
  outStream << "<Style>\n";
  outStream << QString( "<IconStyle>" );
  outStream << QString( "<color>%1</color>" ).arg( color2hex( color() ) );
  // With icon scale=1 google earth normalize the icon to 32 pixels
  outStream << QString( "<scale>%1</scale>" ).arg( iconSize() / 32.0 );
  outStream << QString( "</IconStyle>\n" );
  if ( !mName.isEmpty() )
  {
    outStream << QString( "<LabelStyle>" );
    if ( mLabelColor.isValid() )
    {
      outStream << QString( "<color>%1</color>" ).arg( color2hex( mLabelColor ) );
    }
    if ( mLabelFont.pointSize() > -1 )
    {
      // With label scale=1 google earth normalize the text to 16 pixels.
      // In kadas by default we use point size 10. Dividing by 10 we get
      // a similar text size in both applications
      outStream << QString( "<scale>%1</scale>" ).arg( mLabelFont.pointSize() / 10.0 );
    }
    outStream << QString( "</LabelStyle>\n" );
  }
  outStream << "</Style>\n";
  outStream << "<ExtendedData>\n";
  outStream << "<SchemaData schemaUrl=\"#KadasGeometryItem\">\n";
  outStream << QString( "<SimpleData name=\"icon_type\">%1</SimpleData>\n" ).arg( static_cast<int>( iconSize() ) );
  outStream << QString( "<SimpleData name=\"outline_style\">%1</SimpleData>\n" ).arg( QgsSymbolLayerUtils::encodePenStyle( QPen( strokeColor(), strokeWidth() ).style() ) );
  outStream << QString( "<SimpleData name=\"fill_style\">%1</SimpleData>\n" ).arg( QgsSymbolLayerUtils::encodeBrushStyle( QBrush( color() ).style() ) );
  outStream << "</SchemaData>\n";
  outStream << "</ExtendedData>\n";
  QgsPoint point = QgsPoint( annotationItem()->geometry() );
  point.transform( QgsCoordinateTransform( mCrs, QgsCoordinateReferenceSystem( "EPSG:4326" ), QgsProject::instance() ) );
  outStream << point.asKml( 6 ) << "\n";
  outStream << "</Placemark>\n";
  outStream.flush();
  return outString;
}
