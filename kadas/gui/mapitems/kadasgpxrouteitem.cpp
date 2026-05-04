/***************************************************************************
    kadasgpxrouteitem.cpp
    ---------------------
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
#include <QDomElement>

#include <qgis/qgsrendercontext.h>

#include <qgis/qgscoordinatetransform.h>
#include <qgis/qgslinestring.h>
#include <qgis/qgslinesymbol.h>
#include <qgis/qgslinesymbollayer.h>
#include <qgis/qgsmultilinestring.h>
#include <qgis/qgsproject.h>
#include <qgis/qgssymbollayer.h>

#include "kadas/gui/annotationitems/kadasannotationzindex.h"
#include "kadas/gui/annotationitems/kadasgpxrouteannotationitem.h"
#include "kadas/gui/mapitems/kadasgpxrouteitem.h"


KadasGpxRouteItem::KadasGpxRouteItem( QObject *parent )
  : KadasLineItem( QgsCoordinateReferenceSystem( "EPSG:4326" ), parent )
{
  setEditor( KadasMapItemEditor::GPX_ROUTE );
  mLabelFont.setPointSize( 10 );
  mLabelFont.setBold( true );

  // Default style
  int outlineWidth = 2;
  QColor color = Qt::yellow;

  setOutline( QPen( color, outlineWidth ) );
  setFill( QBrush( color ) );
}

QString KadasGpxRouteItem::exportName() const
{
  if ( name().isEmpty() )
    return itemName();

  return name();
}

void KadasGpxRouteItem::setName( const QString &name )
{
  mName = name;
  QFontMetrics fm( mLabelFont );
  mLabelSize = fm.size( 0, name );
  update();
  emit propertyChanged();
}

void KadasGpxRouteItem::setNumber( const QString &number )
{
  mNumber = number;
  update();
  emit propertyChanged();
}

void KadasGpxRouteItem::setLabelFont( const QFont &labelFont )
{
  mLabelFont = labelFont;
  emit propertyChanged();
}

void KadasGpxRouteItem::setLabelColor( const QColor &labelColor )
{
  mLabelColor = labelColor;
  emit propertyChanged();
}

KadasMapItem::Margin KadasGpxRouteItem::margin() const
{
  Margin m = KadasLineItem::margin();
  if ( !mName.isEmpty() )
  {
    // See rendering routine below
    int maxLabelExtent = std::max( mLabelSize.height() / 4 + mLabelSize.width() + 1, mLabelSize.height() );
    m.right = std::max( m.right, maxLabelExtent );
    m.top = std::max( m.top, maxLabelExtent );
  }
  return m;
}

void KadasGpxRouteItem::render( QgsRenderContext &context ) const
{
  KadasLineItem::render( context );

  // Draw name label at regular 5 * label width distance
  if ( !mName.isEmpty() && !constState()->points.isEmpty() && constState()->points.front().size() >= 2 )
  {
    const QList<KadasItemPos> points = constState()->points.front();

    QPainterPath path;
    path.addText( QPointF( 0, 0 ), mLabelFont, mName );

    double interval = mLabelSize.width() + 150;
    double walkDist = 0;

    QColor bufferColor = ( 0.2126 * mBrush.color().red() + 0.7152 * mBrush.color().green() + 0.0722 * mBrush.color().blue() ) > 128 ? Qt::black : Qt::white;

    for ( int i = 0, n = points.size(); i < n - 1; ++i )
    {
      QPointF p1 = context.mapToPixel().transform( context.coordinateTransform().transform( points[i] ) ).toQPointF();
      QPointF p2 = context.mapToPixel().transform( context.coordinateTransform().transform( points[i + 1] ) ).toQPointF();

      double dist = std::sqrt( ( p2.x() - p1.x() ) * ( p2.x() - p1.x() ) + ( p2.y() - p1.y() ) * ( p2.y() - p1.y() ) );
      while ( walkDist < dist )
      {
        QPointF dir( ( p2.x() - p1.x() ) / dist, ( p2.y() - p1.y() ) / dist );
        double angle = std::atan2( dir.y(), dir.x() ) / M_PI * 180.;
        while ( angle < 0 )
          angle += 360.;
        QPointF nor( dir.y(), -dir.x() );
        if ( angle > 90 && angle < 270 )
        {
          angle -= 180.;
          nor *= -1;
        }
        QPointF pos = p1 + walkDist * dir + 0.25 * mLabelSize.height() * nor;

        context.painter()->save();
        context.painter()->translate( pos );
        context.painter()->rotate( angle );
        context.painter()->setBrush( mBrush );
        context.painter()->setPen( QPen( bufferColor, 2 ) );
        context.painter()->drawPath( path );
        context.painter()->setBrush( QBrush( mLabelColor ) );
        context.painter()->setPen( Qt::NoPen );
        context.painter()->drawPath( path );
        context.painter()->restore();

        walkDist += interval;
      }
      walkDist -= dist;
    }
  }
}

QList<QgsAnnotationItem *> KadasGpxRouteItem::annotationItems( const QgsCoordinateReferenceSystem &crs ) const
{
  QList<QgsAnnotationItem *> items;
  const QgsMultiLineString *mls = geometry();
  if ( !mls || mls->isEmpty() )
    return items;

  std::unique_ptr<QgsMultiLineString> clone( mls->clone() );
  if ( crs.isValid() && mCrs != crs )
  {
    QgsCoordinateTransform ct( mCrs, crs, QgsProject::instance() );
    clone->transform( ct );
  }

  auto makeSymbol = [&]() {
    auto *layer = new QgsSimpleLineSymbolLayer( mPen.color(), mPen.widthF(), mPen.style() );
    layer->setWidthUnit( Qgis::RenderUnit::Pixels );
    return new QgsLineSymbol( QgsSymbolLayerList() << layer );
  };

  const int z = zIndex() ? zIndex() : KadasAnnotationZIndex::GpxRoute;
  for ( int i = 0, n = clone->numGeometries(); i < n; ++i )
  {
    const auto *ls = static_cast<const QgsLineString *>( clone->geometryN( i ) );
    auto *anno = new KadasGpxRouteAnnotationItem( ls->clone() );
    anno->setSymbol( makeSymbol() );
    anno->setName( mName );
    anno->setNumber( mNumber );
    anno->setLabelFont( mLabelFont );
    anno->setLabelColor( mLabelColor );
    anno->setZIndex( z );
    items.append( anno );
  }
  return items;
}

KadasMapItem *KadasGpxRouteItem::_clone() const
{
  KadasGpxRouteItem *item = new KadasGpxRouteItem();
  item->mGeometry = mGeometry->clone();
  item->mPen = mPen;
  item->mBrush = mBrush;
  item->mIconType = mIconType;
  item->mIconSize = mIconSize;
  item->mIconPen = mIconPen;
  item->mIconBrush = mIconBrush;
  item->mName = mName;
  item->mNumber = mNumber;
  item->mLabelFont = mLabelFont;
  item->mLabelColor = mLabelColor;
  return item;
}

void KadasGpxRouteItem::writeXmlPrivate( QDomElement &element ) const
{
  KadasLineItem::writeXmlPrivate( element );
  element.setAttribute( QStringLiteral( "gpx_name" ), mName );
  element.setAttribute( QStringLiteral( "gpx_number" ), mNumber );
  element.setAttribute( QStringLiteral( "label_font" ), mLabelFont.toString() );
  element.setAttribute( QStringLiteral( "label_color" ), mLabelColor.name( QColor::HexArgb ) );
}

void KadasGpxRouteItem::readXmlPrivate( const QDomElement &element )
{
  KadasLineItem::readXmlPrivate( element );
  const bool isLegacy = element.attribute( QStringLiteral( "format_version" ), QStringLiteral( "1" ) ) == QLatin1String( "1" );
  if ( isLegacy )
    return;
  mName = element.attribute( QStringLiteral( "gpx_name" ) );
  mNumber = element.attribute( QStringLiteral( "gpx_number" ) );
  const QString fontStr = element.attribute( QStringLiteral( "label_font" ) );
  if ( !fontStr.isEmpty() )
    mLabelFont.fromString( fontStr );
  const QString colorStr = element.attribute( QStringLiteral( "label_color" ) );
  if ( !colorStr.isEmpty() )
    mLabelColor = QColor( colorStr );
}
