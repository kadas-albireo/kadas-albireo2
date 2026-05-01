/***************************************************************************
    kadasmilxannotationitem.cpp
    ---------------------------
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

#include <QDomDocument>
#include <QDomElement>
#include <QPainter>
#include <QPaintDevice>

#include <qgis/qgscoordinatetransform.h>
#include <qgis/qgsmaptopixel.h>
#include <qgis/qgsproject.h>
#include <qgis/qgsrendercontext.h>

#include "kadas/gui/annotationitems/kadasannotationzindex.h"
#include "kadas/gui/annotationitems/kadasmilxannotationitem.h"
#include "kadas/gui/milx/kadasmilxclient.h"


KadasMilxAnnotationItem::KadasMilxAnnotationItem()
{
  setZIndex( KadasAnnotationZIndex::Milx );
}

QString KadasMilxAnnotationItem::type() const
{
  return itemTypeId();
}

QgsRectangle KadasMilxAnnotationItem::boundingBox() const
{
  if ( mPoints.isEmpty() )
    return QgsRectangle();
  QgsRectangle bbox( mPoints.first(), mPoints.first() );
  for ( const QgsPointXY &p : mPoints )
    bbox.combineExtentWith( p.x(), p.y() );
  return bbox;
}

void KadasMilxAnnotationItem::render( QgsRenderContext &context, QgsFeedback *feedback )
{
  Q_UNUSED( feedback )
  if ( mPoints.isEmpty() || mMssString.isEmpty() )
    return;

  // MilX items are stored in EPSG:4326. Project to the destination CRS, then
  // convert to screen pixels via QgsMapToPixel — same as the legacy
  // KadasMilxItem::toSymbol() / render() path.
  const QgsCoordinateTransform mapCrst( QgsCoordinateReferenceSystem( QStringLiteral( "EPSG:4326" ) ), context.coordinateTransform().destinationCrs(), context.transformContext() );
  QList<QPoint> screenPoints;
  screenPoints.reserve( mPoints.size() );
  for ( const QgsPointXY &pt : mPoints )
  {
    QgsPointXY mapPt = pt;
    try
    {
      mapPt = mapCrst.transform( pt );
    }
    catch ( const QgsCsException & )
    {
      return;
    }
    screenPoints.append( context.mapToPixel().transform( mapPt ).toQPointF().toPoint() );
  }

  // libmss needs the on-screen extent of the visible map area in device pixels.
  const QgsRectangle mapExtent = context.mapExtent();
  const QgsMapToPixel &m2p = context.mapToPixel();
  const QPoint tl = m2p.transform( mapExtent.xMinimum(), mapExtent.yMinimum() ).toQPointF().toPoint();
  const QPoint tr = m2p.transform( mapExtent.xMaximum(), mapExtent.yMinimum() ).toQPointF().toPoint();
  const QPoint bl = m2p.transform( mapExtent.xMinimum(), mapExtent.yMaximum() ).toQPointF().toPoint();
  const QPoint br = m2p.transform( mapExtent.xMaximum(), mapExtent.yMaximum() ).toQPointF().toPoint();
  const int xMin = std::min( { tl.x(), tr.x(), bl.x(), br.x() } );
  const int xMax = std::max( { tl.x(), tr.x(), bl.x(), br.x() } );
  const int yMin = std::min( { tl.y(), tr.y(), bl.y(), br.y() } );
  const int yMax = std::max( { tl.y(), tr.y(), bl.y(), br.y() } );
  const QRect screenExtent = QRect( xMin, yMin, xMax - xMin, yMax - yMin ).normalized();

  const KadasMilxClient::NPointSymbol symbol(
    mMssString,
    screenPoints,
    QList<int>(),
    QList<QPair<int, double>>(),
    /* finalized */ true,
    /* colored */ true
  );
  KadasMilxClient::NPointSymbolGraphic result;
  const int dpi = context.painter() && context.painter()->device() ? context.painter()->device()->logicalDpiX() : 96;
  if ( !KadasMilxClient::updateSymbol( screenExtent, dpi, symbol, KadasMilxClient::globalSymbolSettings(), result, /* returnPoints */ false ) )
  {
    return;
  }

  const QPoint renderPos = symbol.points.front() + result.offset;
  context.painter()->drawImage( renderPos, result.graphic );
}

bool KadasMilxAnnotationItem::writeXml( QDomElement &element, QDomDocument &document, const QgsReadWriteContext &context ) const
{
  Q_UNUSED( context )
  element.setAttribute( QStringLiteral( "kadasMssString" ), mMssString );
  element.setAttribute( QStringLiteral( "kadasMilitaryName" ), mMilitaryName );
  element.setAttribute( QStringLiteral( "kadasSymbolType" ), mSymbolType );
  element.setAttribute( QStringLiteral( "kadasMinNumPoints" ), mMinNumPoints );
  element.setAttribute( QStringLiteral( "kadasHasVariablePoints" ), mHasVariablePoints ? 1 : 0 );

  QDomElement pointsElem = document.createElement( QStringLiteral( "kadasPoints" ) );
  for ( const QgsPointXY &pt : mPoints )
  {
    QDomElement ptElem = document.createElement( QStringLiteral( "p" ) );
    ptElem.setAttribute( QStringLiteral( "x" ), QString::number( pt.x(), 'f', 12 ) );
    ptElem.setAttribute( QStringLiteral( "y" ), QString::number( pt.y(), 'f', 12 ) );
    pointsElem.appendChild( ptElem );
  }
  element.appendChild( pointsElem );

  writeCommonProperties( element, document, context );
  return true;
}

bool KadasMilxAnnotationItem::readXml( const QDomElement &element, const QgsReadWriteContext &context )
{
  mMssString = element.attribute( QStringLiteral( "kadasMssString" ) );
  mMilitaryName = element.attribute( QStringLiteral( "kadasMilitaryName" ) );
  mSymbolType = element.attribute( QStringLiteral( "kadasSymbolType" ) );
  mMinNumPoints = element.attribute( QStringLiteral( "kadasMinNumPoints" ), QStringLiteral( "1" ) ).toInt();
  mHasVariablePoints = element.attribute( QStringLiteral( "kadasHasVariablePoints" ), QStringLiteral( "0" ) ).toInt() != 0;

  mPoints.clear();
  const QDomElement pointsElem = element.firstChildElement( QStringLiteral( "kadasPoints" ) );
  QDomElement ptElem = pointsElem.firstChildElement( QStringLiteral( "p" ) );
  while ( !ptElem.isNull() )
  {
    mPoints.append( QgsPointXY( ptElem.attribute( QStringLiteral( "x" ) ).toDouble(), ptElem.attribute( QStringLiteral( "y" ) ).toDouble() ) );
    ptElem = ptElem.nextSiblingElement( QStringLiteral( "p" ) );
  }

  readCommonProperties( element, context );
  return true;
}

KadasMilxAnnotationItem *KadasMilxAnnotationItem::clone() const
{
  auto *item = new KadasMilxAnnotationItem();
  item->mMssString = mMssString;
  item->mMilitaryName = mMilitaryName;
  item->mSymbolType = mSymbolType;
  item->mMinNumPoints = mMinNumPoints;
  item->mHasVariablePoints = mHasVariablePoints;
  item->mPoints = mPoints;
  item->copyCommonProperties( this );
  return item;
}

KadasMilxAnnotationItem *KadasMilxAnnotationItem::create()
{
  return new KadasMilxAnnotationItem();
}
