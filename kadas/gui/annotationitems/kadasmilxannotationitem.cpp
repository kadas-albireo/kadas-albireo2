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

#include "kadas/gui/annotationitems/kadasannotationzindex.h"
#include "kadas/gui/annotationitems/kadasmilxannotationitem.h"


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
  // Skeleton: rendering is implemented in a later slice (delegates to
  // KadasMilxClient::updateSymbol). No-op for now so empty MilX items load
  // and round-trip through QgsAnnotationLayer XML.
  Q_UNUSED( context )
  Q_UNUSED( feedback )
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
