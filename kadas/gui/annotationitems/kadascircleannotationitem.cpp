/***************************************************************************
    kadascircleannotationitem.cpp
    -----------------------------
    copyright            : (C) 2026 by Denis Rouzaud
    email                : denis at opengis dot ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <cmath>

#include <QDomDocument>
#include <QDomElement>

#include <qgis/qgis.h>
#include <qgis/qgsannotationitemnode.h>
#include <qgis/qgsfillsymbol.h>
#include <qgis/qgsfillsymbollayer.h>
#include <qgis/qgsgeometry.h>
#include <qgis/qgslinestring.h>
#include <qgis/qgspolygon.h>
#include <qgis/qgsrendercontext.h>
#include <qgis/qgssymbollayerutils.h>

#include "kadas/gui/annotationitems/kadascircleannotationitem.h"


KadasCircleAnnotationItem::KadasCircleAnnotationItem( const QgsPointXY &center, const QgsPointXY &ringPoint )
  : QgsAnnotationItem()
  , mCenter( center )
  , mRingPoint( ringPoint )
  , mSymbol( std::make_unique<QgsFillSymbol>() )
{}

KadasCircleAnnotationItem::~KadasCircleAnnotationItem() = default;

QString KadasCircleAnnotationItem::type() const
{
  return itemTypeId();
}

Qgis::AnnotationItemFlags KadasCircleAnnotationItem::flags() const
{
  return Qgis::AnnotationItemFlag::SupportsReferenceScale | Qgis::AnnotationItemFlag::SupportsCallouts;
}

double KadasCircleAnnotationItem::radius() const
{
  const double dx = mRingPoint.x() - mCenter.x();
  const double dy = mRingPoint.y() - mCenter.y();
  return std::sqrt( dx * dx + dy * dy );
}

QgsRectangle KadasCircleAnnotationItem::boundingBox() const
{
  const double r = radius();
  if ( r <= 0 )
    return QgsRectangle( mCenter.x(), mCenter.y(), mCenter.x(), mCenter.y() );
  return QgsRectangle( mCenter.x() - r, mCenter.y() - r, mCenter.x() + r, mCenter.y() + r );
}

QgsRectangle KadasCircleAnnotationItem::boundingBox( QgsRenderContext & ) const
{
  return boundingBox();
}

void KadasCircleAnnotationItem::render( QgsRenderContext &context, QgsFeedback * )
{
  if ( !mSymbol )
    return;
  const double r = radius();
  if ( r <= 0 )
    return;

  constexpr int segments = 64;
  QPolygonF poly;
  poly.reserve( segments + 1 );
  for ( int i = 0; i <= segments; ++i )
  {
    const double a = ( 2.0 * M_PI * i ) / segments;
    const double x = mCenter.x() + r * std::cos( a );
    const double y = mCenter.y() + r * std::sin( a );
    double px = x;
    double py = y;
    context.mapToPixel().transformInPlace( px, py );
    poly << QPointF( px, py );
  }
  mSymbol->startRender( context );
  mSymbol->renderPolygon( poly, nullptr, nullptr, context );
  mSymbol->stopRender( context );
}

bool KadasCircleAnnotationItem::writeXml( QDomElement &element, QDomDocument &document, const QgsReadWriteContext &context ) const
{
  element.setAttribute( QStringLiteral( "cx" ), qgsDoubleToString( mCenter.x() ) );
  element.setAttribute( QStringLiteral( "cy" ), qgsDoubleToString( mCenter.y() ) );
  element.setAttribute( QStringLiteral( "rx" ), qgsDoubleToString( mRingPoint.x() ) );
  element.setAttribute( QStringLiteral( "ry" ), qgsDoubleToString( mRingPoint.y() ) );
  element.appendChild( QgsSymbolLayerUtils::saveSymbol( QStringLiteral( "fillSymbol" ), mSymbol.get(), document, context ) );
  writeCommonProperties( element, document, context );
  return true;
}

bool KadasCircleAnnotationItem::readXml( const QDomElement &element, const QgsReadWriteContext &context )
{
  mCenter = QgsPointXY( element.attribute( QStringLiteral( "cx" ) ).toDouble(), element.attribute( QStringLiteral( "cy" ) ).toDouble() );
  mRingPoint = QgsPointXY( element.attribute( QStringLiteral( "rx" ) ).toDouble(), element.attribute( QStringLiteral( "ry" ) ).toDouble() );

  const QDomElement symbolElem = element.firstChildElement( QStringLiteral( "symbol" ) );
  if ( !symbolElem.isNull() )
    setSymbol( QgsSymbolLayerUtils::loadSymbol<QgsFillSymbol>( symbolElem, context ).release() );

  readCommonProperties( element, context );
  return true;
}

QList<QgsAnnotationItemNode> KadasCircleAnnotationItem::nodesV2( const QgsAnnotationItemEditContext & ) const
{
  return {
    QgsAnnotationItemNode( QgsVertexId( 0, 0, 0 ), mCenter, Qgis::AnnotationItemNodeType::VertexHandle ),
    QgsAnnotationItemNode( QgsVertexId( 0, 0, 1 ), mRingPoint, Qgis::AnnotationItemNodeType::VertexHandle ),
  };
}

KadasCircleAnnotationItem *KadasCircleAnnotationItem::clone() const
{
  auto item = std::make_unique<KadasCircleAnnotationItem>( mCenter, mRingPoint );
  if ( mSymbol )
    item->setSymbol( mSymbol->clone() );
  item->copyCommonProperties( this );
  return item.release();
}

KadasCircleAnnotationItem *KadasCircleAnnotationItem::create()
{
  return new KadasCircleAnnotationItem();
}

const QgsFillSymbol *KadasCircleAnnotationItem::symbol() const
{
  return mSymbol.get();
}

void KadasCircleAnnotationItem::setSymbol( QgsFillSymbol *symbol )
{
  mSymbol.reset( symbol );
}
