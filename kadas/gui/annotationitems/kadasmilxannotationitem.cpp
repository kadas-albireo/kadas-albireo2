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
#include <QVector2D>

#include <qgis/qgscoordinatetransform.h>
#include <qgis/qgsdistancearea.h>
#include <qgis/qgsmapsettings.h>
#include <qgis/qgsmaptopixel.h>
#include <qgis/qgsproject.h>
#include <qgis/qgsrendercontext.h>
#include <qgis/qgsunittypes.h>

#include "kadas/gui/annotationitems/kadasannotationzindex.h"
#include "kadas/gui/annotationitems/kadasmilxannotationitem.h"
#include "kadas/gui/annotationitems/kadasmilxlayersettings.h"
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

  // Thread-safety: this method may be invoked from a non-GUI thread
  // (3D / async map renderers). All KadasMilxClient calls funnel through
  // KadasMilxClient::processRequest(), which dispatches via
  // Qt::BlockingQueuedConnection to its own worker thread when invoked
  // outside the GUI thread; the lazy `instance()` getter is mutex-guarded.
  // See kadasmilxclient.cpp for the dispatch logic.

  // Build a transient QgsMapSettings from the render context so we can reuse
  // toSymbol() / computeScreenExtent() (the libmss IPC path is shared with
  // the controller's draw/edit state machine).
  QgsMapSettings ms;
  ms.setDestinationCrs( context.coordinateTransform().destinationCrs() );
  ms.setExtent( context.mapExtent() );
  ms.setOutputSize( QSize( context.mapToPixel().mapWidth(), context.mapToPixel().mapHeight() ) );
  ms.setOutputDpi( context.painter() && context.painter()->device() ? context.painter()->device()->logicalDpiX() : 96 );
  ms.setTransformContext( context.transformContext() );

  KadasMilxClient::NPointSymbolGraphic result;
  KadasMilxClient::NPointSymbol symbol = [&]() {
    try
    {
      return toSymbol( ms, /* colored */ true );
    }
    catch ( const QgsCsException & )
    {
      return KadasMilxClient::NPointSymbol( QString(), {}, {}, {}, false, false );
    }
  }();
  if ( symbol.points.isEmpty() || symbol.xml.isEmpty() )
    return;

  if ( !KadasMilxClient::updateSymbol( computeScreenExtent( ms ), ms.outputDpi(), symbol, KadasMilxLayerSettings::resolve( context ), result, /* returnPoints */ false ) )
  {
    return;
  }

  const QPoint renderPos = symbol.points.front() + result.offset + mUserOffset;
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

  // Control point indices reported by libmss (rendered as red ctrl handles,
  // omitted from geometry export). Persisted so that opening a saved project
  // does not require an immediate libmss round-trip just to redraw nodes.
  QDomElement ctrlElem = document.createElement( QStringLiteral( "kadasControlPoints" ) );
  for ( int idx : mControlPoints )
  {
    QDomElement cElem = document.createElement( QStringLiteral( "i" ) );
    cElem.setAttribute( QStringLiteral( "v" ), idx );
    ctrlElem.appendChild( cElem );
  }
  element.appendChild( ctrlElem );

  // Symbol attributes (width / length / radius / attitude), keyed by libmss
  // attribute id; values are in WGS84 metres / degrees.
  QDomElement attrsElem = document.createElement( QStringLiteral( "kadasAttributes" ) );
  for ( auto it = mAttributes.cbegin(), itEnd = mAttributes.cend(); it != itEnd; ++it )
  {
    QDomElement aElem = document.createElement( QStringLiteral( "a" ) );
    aElem.setAttribute( QStringLiteral( "k" ), it.key() );
    aElem.setAttribute( QStringLiteral( "v" ), QString::number( it.value(), 'f', 12 ) );
    attrsElem.appendChild( aElem );
  }
  element.appendChild( attrsElem );

  QDomElement attrPtsElem = document.createElement( QStringLiteral( "kadasAttributePoints" ) );
  for ( auto it = mAttributePoints.cbegin(), itEnd = mAttributePoints.cend(); it != itEnd; ++it )
  {
    QDomElement aElem = document.createElement( QStringLiteral( "ap" ) );
    aElem.setAttribute( QStringLiteral( "k" ), it.key() );
    aElem.setAttribute( QStringLiteral( "x" ), QString::number( it.value().x(), 'f', 12 ) );
    aElem.setAttribute( QStringLiteral( "y" ), QString::number( it.value().y(), 'f', 12 ) );
    attrPtsElem.appendChild( aElem );
  }
  element.appendChild( attrPtsElem );

  element.setAttribute( QStringLiteral( "kadasUserOffsetX" ), mUserOffset.x() );
  element.setAttribute( QStringLiteral( "kadasUserOffsetY" ), mUserOffset.y() );

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

  mControlPoints.clear();
  const QDomElement ctrlElem = element.firstChildElement( QStringLiteral( "kadasControlPoints" ) );
  QDomElement cElem = ctrlElem.firstChildElement( QStringLiteral( "i" ) );
  while ( !cElem.isNull() )
  {
    mControlPoints.append( cElem.attribute( QStringLiteral( "v" ) ).toInt() );
    cElem = cElem.nextSiblingElement( QStringLiteral( "i" ) );
  }

  mAttributes.clear();
  const QDomElement attrsElem = element.firstChildElement( QStringLiteral( "kadasAttributes" ) );
  QDomElement aElem = attrsElem.firstChildElement( QStringLiteral( "a" ) );
  while ( !aElem.isNull() )
  {
    mAttributes.insert( static_cast<KadasMilxAttrType>( aElem.attribute( QStringLiteral( "k" ) ).toInt() ), aElem.attribute( QStringLiteral( "v" ) ).toDouble() );
    aElem = aElem.nextSiblingElement( QStringLiteral( "a" ) );
  }

  mAttributePoints.clear();
  const QDomElement attrPtsElem = element.firstChildElement( QStringLiteral( "kadasAttributePoints" ) );
  QDomElement apElem = attrPtsElem.firstChildElement( QStringLiteral( "ap" ) );
  while ( !apElem.isNull() )
  {
    mAttributePoints.insert(
      static_cast<KadasMilxAttrType>( apElem.attribute( QStringLiteral( "k" ) ).toInt() ),
      QgsPointXY( apElem.attribute( QStringLiteral( "x" ) ).toDouble(), apElem.attribute( QStringLiteral( "y" ) ).toDouble() )
    );
    apElem = apElem.nextSiblingElement( QStringLiteral( "ap" ) );
  }

  mUserOffset = QPoint( element.attribute( QStringLiteral( "kadasUserOffsetX" ), QStringLiteral( "0" ) ).toInt(), element.attribute( QStringLiteral( "kadasUserOffsetY" ), QStringLiteral( "0" ) ).toInt() );

  // Items loaded from XML are by definition finalized.
  mDrawStatus = DrawStatus::Finished;
  mPressedPoints = mPoints.size();

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
  item->mControlPoints = mControlPoints;
  item->mAttributes = mAttributes;
  item->mAttributePoints = mAttributePoints;
  item->mUserOffset = mUserOffset;
  item->mPressedPoints = mPressedPoints;
  item->mDrawStatus = mDrawStatus;
  item->copyCommonProperties( this );
  return item;
}

KadasMilxAnnotationItem *KadasMilxAnnotationItem::create()
{
  return new KadasMilxAnnotationItem();
}

// ----- libmss helpers -------------------------------------------------------

QRect KadasMilxAnnotationItem::computeScreenExtent( const QgsMapSettings &mapSettings )
{
  const QgsRectangle mapExtent = mapSettings.visibleExtent();
  const QgsMapToPixel &m2p = mapSettings.mapToPixel();
  const QPoint tl = m2p.transform( mapExtent.xMinimum(), mapExtent.yMinimum() ).toQPointF().toPoint();
  const QPoint tr = m2p.transform( mapExtent.xMaximum(), mapExtent.yMinimum() ).toQPointF().toPoint();
  const QPoint bl = m2p.transform( mapExtent.xMinimum(), mapExtent.yMaximum() ).toQPointF().toPoint();
  const QPoint br = m2p.transform( mapExtent.xMaximum(), mapExtent.yMaximum() ).toQPointF().toPoint();
  const int xMin = std::min( { tl.x(), tr.x(), bl.x(), br.x() } );
  const int xMax = std::max( { tl.x(), tr.x(), bl.x(), br.x() } );
  const int yMin = std::min( { tl.y(), tr.y(), bl.y(), br.y() } );
  const int yMax = std::max( { tl.y(), tr.y(), bl.y(), br.y() } );
  return QRect( xMin, yMin, xMax - xMin, yMax - yMin ).normalized();
}

double KadasMilxAnnotationItem::metersToPixels( const QgsMapSettings &mapSettings ) const
{
  if ( mPoints.isEmpty() )
    return 1.0;

  QgsDistanceArea da;
  da.setSourceCrs( QgsCoordinateReferenceSystem( QStringLiteral( "EPSG:4326" ) ), mapSettings.transformContext() );
  da.setEllipsoid( QStringLiteral( "WGS84" ) );

  const QgsCoordinateTransform mapCrst( QgsCoordinateReferenceSystem( QStringLiteral( "EPSG:4326" ) ), mapSettings.destinationCrs(), mapSettings.transformContext() );
  const QgsPointXY point = mPoints.front();
  const QgsPointXY otherPoint( point.x() + 0.001, point.y() );
  const QPointF screenPoint = mapSettings.mapToPixel().transform( mapCrst.transform( point ) ).toQPointF();
  const QPointF otherScreenPoint = mapSettings.mapToPixel().transform( mapCrst.transform( otherPoint ) ).toQPointF();
  const double ellipsoidDist = da.measureLine( point, otherPoint ) * QgsUnitTypes::fromUnitToUnitFactor( da.lengthUnits(), Qgis::DistanceUnit::Meters );
  const double screenDist = QVector2D( screenPoint - otherScreenPoint ).length();
  if ( ellipsoidDist <= 0 )
    return 1.0;
  return screenDist / ellipsoidDist;
}

KadasMilxClient::NPointSymbol KadasMilxAnnotationItem::toSymbol( const QgsMapSettings &mapSettings, bool colored ) const
{
  const QgsCoordinateTransform mapCrst( QgsCoordinateReferenceSystem( QStringLiteral( "EPSG:4326" ) ), mapSettings.destinationCrs(), mapSettings.transformContext() );

  QList<QPoint> screenPoints;
  screenPoints.reserve( mPoints.size() );
  for ( const QgsPointXY &p : mPoints )
  {
    screenPoints.append( mapSettings.mapToPixel().transform( mapCrst.transform( p ) ).toQPointF().toPoint() );
  }

  QList<QPair<int, double>> screenAttribs;
  if ( !mAttributes.isEmpty() )
  {
    const double m2p = metersToPixels( mapSettings );
    for ( auto it = mAttributes.cbegin(), itEnd = mAttributes.cend(); it != itEnd; ++it )
    {
      double value = it.value();
      if ( it.key() != MilxAttributeAttitude )
      {
        value *= m2p;
      }
      screenAttribs.append( qMakePair( it.key(), value ) );
    }
  }

  const bool finalized = mDrawStatus == DrawStatus::Finished;
  return KadasMilxClient::NPointSymbol( mMssString, screenPoints, mControlPoints, screenAttribs, finalized, colored );
}

void KadasMilxAnnotationItem::applySymbolResult( const QgsMapSettings &mapSettings, const KadasMilxClient::NPointSymbolGraphic &result )
{
  const QgsCoordinateTransform mapCrst( QgsCoordinateReferenceSystem( QStringLiteral( "EPSG:4326" ) ), mapSettings.destinationCrs(), mapSettings.transformContext() );

  mPoints.clear();
  for ( const QPoint &screenPoint : result.adjustedPoints )
  {
    const QgsPointXY mapPt = mapSettings.mapToPixel().toMapCoordinates( screenPoint );
    const QgsPointXY itemPt = mapCrst.transform( mapPt, Qgis::TransformDirection::Reverse );
    mPoints.append( itemPt );
  }

  mControlPoints = result.controlPoints;

  mAttributes.clear();
  if ( !mPoints.isEmpty() )
  {
    const double m2p = metersToPixels( mapSettings );
    for ( auto it = result.attributes.cbegin(), itEnd = result.attributes.cend(); it != itEnd; ++it )
    {
      double value = it.value();
      if ( it.key() != MilxAttributeAttitude && m2p != 0.0 )
      {
        value /= m2p;
      }
      mAttributes.insert( it.key(), value );
    }
  }

  mAttributePoints.clear();
  for ( auto it = result.attributePoints.cbegin(), itEnd = result.attributePoints.cend(); it != itEnd; ++it )
  {
    const QgsPointXY mapPt = mapSettings.mapToPixel().toMapCoordinates( it.value() );
    const QgsPointXY itemPt = mapCrst.transform( mapPt, Qgis::TransformDirection::Reverse );
    mAttributePoints.insert( it.key(), itemPt );
  }
}
