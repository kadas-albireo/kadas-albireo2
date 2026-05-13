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
#include <QDomElement>

#include <qgis/qgsannotationpointtextitem.h>
#include <qgis/qgsfeedback.h>
#include <qgis/qgsproject.h>
#include <qgis/qgstextformat.h>

#include "kadas/gui/mapitems/kadastextitem.h"


KadasTextItem::KadasTextItem( const QgsCoordinateReferenceSystem &crs )
  : KadasTextItem( crs, new QgsAnnotationPointTextItem( QString(), QgsPoint() ) )
{}

KadasTextItem::KadasTextItem( const QgsCoordinateReferenceSystem &crs, QgsAnnotationPointTextItem *qgsItem )
  : KadasAbstractPointItem( crs )
  , mQgsItem( qgsItem )
{
  connect( this, &KadasMapItem::zIndexChanged, this, [=, this]( int index ) { mQgsItem->setZIndex( index ); } );
}

KadasTextItem::~KadasTextItem()
{
  delete mQgsItem;
}

void KadasTextItem::setText( const QString &text )
{
  mText = text;
  mQgsItem->setText( text );
  update();
}

void KadasTextItem::setItemGeometry( const QgsPointXY &point )
{
  mQgsItem->setPoint( QgsPoint( point ) );
  update();
}

QgsAnnotationPointTextItem *KadasTextItem::annotationItem( const QgsCoordinateReferenceSystem &crs ) const
{
  QgsAnnotationPointTextItem *item = mQgsItem->clone();
  if ( mCrs != crs )
  {
    QgsCoordinateTransform ct( mCrs, crs, QgsProject::instance() );
    item->setPoint( ct.transform( item->point() ) );
  }
  return item;
}

void KadasTextItem::setFont( const QFont &font )
{
  mFont = font;
  updateQgsAnnotation();
}

void KadasTextItem::setColor( const QColor &color )
{
  mColor = color;
  updateQgsAnnotation();
}

void KadasTextItem::setOutlineColor( const QColor &color )
{
  mOutlineColor = color;
  updateQgsAnnotation();
}

void KadasTextItem::setAngle( double angle )
{
  mAngle = angle;
  mQgsItem->setAngle( angle );
  update();
  emit propertyChanged();
}


void KadasTextItem::updateQgsAnnotation()
{
  QgsTextFormat fmt;
  fmt.setFont( mFont );
  fmt.setSize( mFont.pointSize() );
  fmt.setSizeUnit( Qgis::RenderUnit::Points );
  fmt.setColor( mColor );
  if ( mOutlineColor.isValid() )
  {
    QgsTextBufferSettings buffer;
    buffer.setEnabled( true );
    buffer.setColor( mOutlineColor );
    buffer.setOpacity( mOutlineColor.alpha() / 255.0 );
    buffer.setSize( 1 );
    fmt.setBuffer( buffer );
  }
  mQgsItem->setFormat( fmt );
  mQgsItem->setAngle( mAngle );
  update();
}

KadasMapItem *KadasTextItem::_clone() const SIP_FACTORY
{
  KadasTextItem *item = new KadasTextItem( crs(), mQgsItem->clone() );
  item->mText = mText;
  item->mFont = mFont;
  item->mColor = mColor;
  item->mOutlineColor = mOutlineColor;
  item->mAngle = mAngle;
  return item;
}

void KadasTextItem::writeXmlPrivate( QDomElement &element ) const
{
  element.setAttribute( QStringLiteral( "text" ), mText );
  element.setAttribute( QStringLiteral( "color" ), mColor.name( QColor::HexArgb ) );
  element.setAttribute( QStringLiteral( "outline_color" ), mOutlineColor.name( QColor::HexArgb ) );
  element.setAttribute( QStringLiteral( "font" ), mFont.toString() );
  element.setAttribute( QStringLiteral( "angle" ), mAngle );
  element.setAttribute( QStringLiteral( "geometry" ), QgsPoint( point() ).asWkt() );
}

void KadasTextItem::readXmlPrivate( const QDomElement &element )
{
  mText = element.attribute( QStringLiteral( "text" ) );
  const QString colorStr = element.attribute( QStringLiteral( "color" ) );
  if ( !colorStr.isEmpty() )
    mColor = QColor( colorStr );
  const QString outlineStr = element.attribute( QStringLiteral( "outline_color" ) );
  if ( !outlineStr.isEmpty() )
    mOutlineColor = QColor( outlineStr );
  const QString fontStr = element.attribute( QStringLiteral( "font" ) );
  if ( !fontStr.isEmpty() )
    mFont.fromString( fontStr );
  mAngle = element.attribute( QStringLiteral( "angle" ), QStringLiteral( "0" ) ).toDouble();
  const QString wkt = element.attribute( QStringLiteral( "geometry" ) );
  if ( !wkt.isEmpty() )
  {
    QgsGeometry g = QgsGeometry::fromWkt( wkt );
    if ( !g.isNull() )
      setPoint( g.asPoint() );
  }
  updateQgsAnnotation();
}

QgsRectangle KadasTextItem::boundingBox() const
{
  return mQgsItem->boundingBox();
}

void KadasTextItem::render( QgsRenderContext &context ) const
{
  QgsFeedback fb;
  mQgsItem->render( context, &fb );
}


QString KadasTextItem::asKml( const QgsRenderContext &context, QuaZip *kmzZip ) const
{
  auto color2hex = []( const QColor &c ) {
    return QString( "%1%2%3%4" ).arg( c.alpha(), 2, 16, QChar( '0' ) ).arg( c.blue(), 2, 16, QChar( '0' ) ).arg( c.green(), 2, 16, QChar( '0' ) ).arg( c.red(), 2, 16, QChar( '0' ) );
  };
  QgsPointXY pos = QgsCoordinateTransform( mCrs, QgsCoordinateReferenceSystem( "EPSG:4326" ), QgsProject::instance() ).transform( position() );

  QString outString;
  QTextStream outStream( &outString );
  outStream << "<Placemark>" << "\n";
  outStream << QString( "<name>%1</name>\n" ).arg( mText );
  outStream << "<Style>";
  outStream << QString( "<IconStyle><scale>0</scale></IconStyle><LabelStyle><color>%1</color><scale>%2</scale></LabelStyle></Style>" ).arg( color2hex( mColor ) ).arg( mFont.pointSizeF() / QFont().pointSizeF() );
  outStream << QString( "<Point><coordinates>%1,%2</coordinates></Point>" ).arg( QString::number( pos.x(), 'f', 10 ) ).arg( QString::number( pos.y(), 'f', 10 ) );
  outStream << "</Placemark>" << "\n";
  outStream.flush();
  return outString;
}
