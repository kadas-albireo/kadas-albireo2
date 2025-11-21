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
#include <QJsonArray>
#include <QMenu>

#include <qgis/qgsannotationpointtextitem.h>
#include <qgis/qgsfeedback.h>
#include <qgis/qgsproject.h>

#include "kadas/gui/mapitems/kadastextitem.h"


KadasTextItem::KadasTextItem( const QgsCoordinateReferenceSystem &crs )
  : KadasPointItem( crs )
{
  mQgsItem = new QgsAnnotationPointTextItem( QString(), QgsPoint() );
}

void KadasTextItem::setText( const QString &text )
{
  mText = text;
  mQgsItem->setText( text );
  update();
}

QgsPointXY KadasTextItem::point() const
{
  return mQgsItem->point();
}

void KadasTextItem::setPoint( const QgsPointXY &point )
{
  mQgsItem->setPoint( QgsPoint( point ) );
  update();
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


void KadasTextItem::updateQgsAnnotation()
{
  QgsTextFormat fmt;
  fmt.setFont( mFont );
  fmt.setSize( mFont.pointSize() );
  fmt.setSizeUnit( Qgis::RenderUnit::Points );
  fmt.setColor( mColor );
  mQgsItem->setFormat( fmt );
  update();
}

KadasMapItem *KadasTextItem::_clone() const SIP_FACTORY
{
  KadasTextItem *item = new KadasTextItem( crs() );
  item->mQgsItem = mQgsItem->clone();
  item->mText = mText;
  item->mFont = mFont;
  item->mColor = mColor;
  return item;
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


void KadasTextItem::writeXmlPrivate( QDomElement &element ) const
{
  //element.setAttribute("status", static_cast<int>( state().drawStatus ));
  element.setAttribute( "text", mText );
  element.setAttribute( "font", mFont.toString() );
  element.setAttribute( "color", mColor.name() );
  element.setAttribute( "geometry", point().asWkt() );
}

void KadasTextItem::readXmlPrivate( const QDomElement &element )
{
  if ( !element.hasAttribute( "text" ) )
  {
    // migration code
    QJsonObject data = QJsonDocument::fromJson( element.firstChild().toCDATASection().data().toLocal8Bit() ).object();
    if ( data.contains( "props" ) )
    {
      QJsonObject props = data["props"].toObject();

      mCrs = QgsCoordinateReferenceSystem( props.value( "authId" ).toString() );
      mEditor = props.value( "editor" ).toString();

      setText( props.value( "text" ).toString() );
      mColor = QColor( props.value( "fillColor" ).toString() );
      mFont.fromString( props.value( "font" ).toString() );
    }
    if ( data.contains( "state" ) )
    {
      const QJsonArray point = data.value( "state" ).toObject().value( "pos" ).toArray();
      setPoint( QgsPointXY( point.at( 0 ).toDouble(), point.at( 1 ).toDouble() ) );
    }
  }
  else
  {
    setText( element.attribute( "text" ) );
    mColor = QColor( element.attribute( "color", QColor( Qt::red ).name() ) );
    mFont.fromString( element.attribute( "font" ) );
    setPoint( QgsGeometry::fromWkt( element.attribute( "geometry" ) ).asPoint() );
  }
  updateQgsAnnotation();
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
  outStream << QString( "<IconStyle><scale>0</scale></IconStyle><LabelStyle><color>%1</color><scale>%2</scale></LabelStyle></Style>" ).arg( color2hex( mColor ) ).arg( mFont.pointSizeF() / QFont().pointSizeF() );
  outStream << QString( "<Point><coordinates>%1,%2</coordinates></Point>" ).arg( QString::number( pos.x(), 'f', 10 ) ).arg( QString::number( pos.y(), 'f', 10 ) );
  outStream << "</Placemark>" << "\n";
  outStream.flush();
  return outString;
}
