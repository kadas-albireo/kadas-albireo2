/***************************************************************************
    kadasrectangleannotationitem.cpp
    --------------------------------
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

#include <qgis/qgis.h>
#include <qgis/qgscoordinatetransform.h>
#include <qgis/qgsexception.h>
#include <qgis/qgsfillsymbol.h>
#include <qgis/qgslinestring.h>
#include <qgis/qgspoint.h>
#include <qgis/qgspolygon.h>
#include <qgis/qgsproject.h>

#include "kadas/gui/annotationitems/kadasannotationzindex.h"
#include "kadas/gui/annotationitems/kadasannotationshadow.h"
#include "kadas/gui/annotationitems/kadasrectangleannotationitem.h"


KadasRectangleAnnotationItem::KadasRectangleAnnotationItem( const QgsPointXY &center, const QSizeF &size, double angleDegrees )
  : QgsAnnotationPolygonItem( new QgsPolygon() )
  , mCenter( center )
  , mSize( size )
  , mAngle( angleDegrees )
{
  setZIndex( KadasAnnotationZIndex::Rectangle );
  rebuildGeometry();
}

QString KadasRectangleAnnotationItem::type() const
{
  return itemTypeId();
}

QVector<QgsPointXY> KadasRectangleAnnotationItem::corners() const
{
  const bool needTransform = mDrawCrs.isValid() && mLayerCrs.isValid() && mDrawCrs != mLayerCrs;

  QgsPointXY centerDraw = mCenter;
  if ( needTransform )
  {
    try
    {
      centerDraw = QgsCoordinateTransform( mLayerCrs, mDrawCrs, QgsProject::instance()->transformContext() ).transform( mCenter );
    }
    catch ( const QgsCsException & )
    {
      centerDraw = mCenter;
    }
  }

  const double halfW = 0.5 * mSize.width();
  const double halfH = 0.5 * mSize.height();
  const double localX[4] = { -halfW, halfW, halfW, -halfW };
  const double localY[4] = { -halfH, -halfH, halfH, halfH };

  const double a = mAngle * M_PI / 180.0;
  const double cosA = std::cos( a );
  const double sinA = std::sin( a );

  QVector<QgsPointXY> drawCorners;
  drawCorners.reserve( 4 );
  for ( int i = 0; i < 4; ++i )
  {
    const double rx = cosA * localX[i] - sinA * localY[i];
    const double ry = sinA * localX[i] + cosA * localY[i];
    drawCorners.append( QgsPointXY( centerDraw.x() + rx, centerDraw.y() + ry ) );
  }

  if ( !needTransform )
    return drawCorners;

  QVector<QgsPointXY> layerCorners;
  layerCorners.reserve( 4 );
  QgsCoordinateTransform ct( mDrawCrs, mLayerCrs, QgsProject::instance()->transformContext() );
  for ( const QgsPointXY &p : drawCorners )
  {
    try
    {
      layerCorners.append( ct.transform( p ) );
    }
    catch ( const QgsCsException & )
    {
      layerCorners.append( p );
    }
  }
  return layerCorners;
}

QgsPointXY KadasRectangleAnnotationItem::rotationHandle() const
{
  const bool needTransform = mDrawCrs.isValid() && mLayerCrs.isValid() && mDrawCrs != mLayerCrs;

  QgsPointXY centerDraw = mCenter;
  if ( needTransform )
  {
    try
    {
      centerDraw = QgsCoordinateTransform( mLayerCrs, mDrawCrs, QgsProject::instance()->transformContext() ).transform( mCenter );
    }
    catch ( const QgsCsException & )
    {
      centerDraw = mCenter;
    }
  }

  const double halfH = 0.5 * mSize.height();
  const double localX = 0.0;
  const double localY = halfH + 0.25 * std::abs( mSize.height() );

  const double a = mAngle * M_PI / 180.0;
  const double cosA = std::cos( a );
  const double sinA = std::sin( a );
  const double rx = cosA * localX - sinA * localY;
  const double ry = sinA * localX + cosA * localY;
  const QgsPointXY drawHandle( centerDraw.x() + rx, centerDraw.y() + ry );

  if ( !needTransform )
    return drawHandle;

  try
  {
    return QgsCoordinateTransform( mDrawCrs, mLayerCrs, QgsProject::instance()->transformContext() ).transform( drawHandle );
  }
  catch ( const QgsCsException & )
  {
    return drawHandle;
  }
}

void KadasRectangleAnnotationItem::rebuildGeometry()
{
  if ( mSize.isEmpty() )
  {
    setGeometry( new QgsPolygon() );
    return;
  }
  const QVector<QgsPointXY> c = corners();
  auto *ring = new QgsLineString();
  for ( const QgsPointXY &p : c )
    ring->addVertex( QgsPoint( p.x(), p.y() ) );
  ring->addVertex( QgsPoint( c.first().x(), c.first().y() ) );
  auto *poly = new QgsPolygon();
  poly->setExteriorRing( ring );
  setGeometry( poly );
}

void KadasRectangleAnnotationItem::setBox( const QgsPointXY &center, const QSizeF &size, double angleDegrees )
{
  mCenter = center;
  mSize = size;
  mAngle = angleDegrees;
  mDrawCrs = QgsCoordinateReferenceSystem();
  mLayerCrs = QgsCoordinateReferenceSystem();
  rebuildGeometry();
}

void KadasRectangleAnnotationItem::setBox( const QgsPointXY &center, const QSizeF &size, double angleDegrees, const QgsCoordinateReferenceSystem &drawCrs, const QgsCoordinateReferenceSystem &layerCrs )
{
  mCenter = center;
  mSize = size;
  mAngle = angleDegrees;
  mDrawCrs = drawCrs;
  mLayerCrs = layerCrs;
  rebuildGeometry();
}

void KadasRectangleAnnotationItem::setCenter( const QgsPointXY &center )
{
  mCenter = center;
  rebuildGeometry();
}

void KadasRectangleAnnotationItem::setSize( const QSizeF &size )
{
  mSize = size;
  rebuildGeometry();
}

void KadasRectangleAnnotationItem::setAngle( double angleDegrees )
{
  mAngle = angleDegrees;
  rebuildGeometry();
}

bool KadasRectangleAnnotationItem::writeXml( QDomElement &element, QDomDocument &document, const QgsReadWriteContext &context ) const
{
  QgsAnnotationPolygonItem::writeXml( element, document, context );
  element.setAttribute( QStringLiteral( "cx" ), qgsDoubleToString( mCenter.x() ) );
  element.setAttribute( QStringLiteral( "cy" ), qgsDoubleToString( mCenter.y() ) );
  element.setAttribute( QStringLiteral( "w" ), qgsDoubleToString( mSize.width() ) );
  element.setAttribute( QStringLiteral( "h" ), qgsDoubleToString( mSize.height() ) );
  element.setAttribute( QStringLiteral( "angle" ), qgsDoubleToString( mAngle ) );
  if ( mDrawCrs.isValid() )
  {
    element.setAttribute( QStringLiteral( "drawCrs" ), mDrawCrs.authid() );
    element.setAttribute( QStringLiteral( "drawCrsWkt" ), mDrawCrs.toWkt() );
  }
  if ( mLayerCrs.isValid() )
  {
    element.setAttribute( QStringLiteral( "layerCrs" ), mLayerCrs.authid() );
    element.setAttribute( QStringLiteral( "layerCrsWkt" ), mLayerCrs.toWkt() );
  }
  mShadow.writeXml( element );
  return true;
}

bool KadasRectangleAnnotationItem::readXml( const QDomElement &element, const QgsReadWriteContext &context )
{
  QgsAnnotationPolygonItem::readXml( element, context );
  mCenter = QgsPointXY( element.attribute( QStringLiteral( "cx" ) ).toDouble(), element.attribute( QStringLiteral( "cy" ) ).toDouble() );
  mSize = QSizeF( element.attribute( QStringLiteral( "w" ) ).toDouble(), element.attribute( QStringLiteral( "h" ) ).toDouble() );
  mAngle = element.attribute( QStringLiteral( "angle" ) ).toDouble();

  auto restoreCrs = []( const QDomElement &el, const QString &authidAttr, const QString &wktAttr ) {
    QgsCoordinateReferenceSystem crs;
    const QString authid = el.attribute( authidAttr );
    if ( !authid.isEmpty() )
      crs = QgsCoordinateReferenceSystem( authid );
    if ( !crs.isValid() )
    {
      const QString wkt = el.attribute( wktAttr );
      if ( !wkt.isEmpty() )
        crs = QgsCoordinateReferenceSystem::fromWkt( wkt );
    }
    return crs;
  };
  mDrawCrs = restoreCrs( element, QStringLiteral( "drawCrs" ), QStringLiteral( "drawCrsWkt" ) );
  mLayerCrs = restoreCrs( element, QStringLiteral( "layerCrs" ), QStringLiteral( "layerCrsWkt" ) );

  mShadow.readXml( element );
  rebuildGeometry();
  return true;
}

KadasRectangleAnnotationItem *KadasRectangleAnnotationItem::clone() const
{
  auto *item = new KadasRectangleAnnotationItem( mCenter, mSize, mAngle );
  item->mDrawCrs = mDrawCrs;
  item->mLayerCrs = mLayerCrs;
  item->rebuildGeometry();
  if ( symbol() )
    item->setSymbol( symbol()->clone() );
  item->copyCommonProperties( this );
  return item;
}

KadasRectangleAnnotationItem *KadasRectangleAnnotationItem::create()
{
  return new KadasRectangleAnnotationItem();
}
