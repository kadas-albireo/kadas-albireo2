/***************************************************************************
    kdasmilxitem.cpp
    ----------------
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

#include <QDesktopWidget>
#include <QVector2D>

#include <qgis/qgsdistancearea.h>
#include <qgis/qgsmapsettings.h>
#include <qgis/qgsmaptopixel.h>
#include <qgis/qgsproject.h>

#include <kadas/app/kadasapplication.h>
#include <kadas/app/milx/kadasmilxitem.h>


KadasMilxItem::KadasMilxItem( QObject *parent )
  : KadasMapItem( QgsCoordinateReferenceSystem( "EPSG:4326" ), parent )
{
  clear();
}

void KadasMilxItem::setSymbol( const KadasMilxClient::SymbolDesc &symbolDesc )
{
  mMssString = symbolDesc.symbolXml;
  mMilitaryName = symbolDesc.militaryName;
  mHasVariablePoints = symbolDesc.hasVariablePoints;
  mMinNPoints = symbolDesc.minNumPoints;
}

QgsRectangle KadasMilxItem::boundingBox() const
{
  QgsRectangle r;
  for ( const QgsPointXY &p : constState()->points )
  {
    if ( r.isNull() )
    {
      r.set( p.x(), p.y(), p.x(), p.y() );
    }
    else
    {
      r.include( p );
    }
  }
  return r;
}

QRect KadasMilxItem::margin() const
{
  return mMargin;
}

QList<KadasMapItem::Node> KadasMilxItem::nodes( const QgsMapSettings &settings ) const
{
  QList<Node> nodes;
  for ( int i = 0, n = constState()->points.size(); i < n; ++i )
  {
    if ( constState()->controlPoints.contains( i ) )
    {
      nodes.append( {constState()->points[i], ctrlPointNodeRenderer} );
    }
    else
    {
      nodes.append( {constState()->points[i], posPointNodeRenderer} );
    }
  }
  for ( const auto attributePoint : constState()->attributePoints )
  {
    nodes.append( {attributePoint.second, ctrlPointNodeRenderer} );
  }
  return nodes;
}

bool KadasMilxItem::intersects( const QgsRectangle &rect, const QgsMapSettings &settings ) const
{
  QgsCoordinateTransform crst( mCrs, settings.destinationCrs(), QgsProject::instance()->transformContext() );
  QPoint screenPos = settings.mapToPixel().transform( crst.transform( rect.center() ) ).toQPointF().toPoint();
  int selectedSymbol = -1;
  QList<KadasMilxClient::NPointSymbol> symbols;
  symbols.append( toSymbol( settings.mapToPixel(), settings.destinationCrs() ) );
  QRect bbox;
  return KadasMilxClient::pickSymbol( symbols, screenPos, selectedSymbol, bbox ) && selectedSymbol >= 0;
}

void KadasMilxItem::render( QgsRenderContext &context ) const
{
  KadasMilxClient::NPointSymbol symbol = toSymbol( context.mapToPixel(), context.coordinateTransform().destinationCrs() );
  KadasMilxClient::NPointSymbolGraphic result;

  int dpi = context.painter()->device()->logicalDpiX();
  QRect screenExtent = computeScreenExtent( context.mapExtent(), context.mapToPixel() );
  if ( !KadasMilxClient::updateSymbol( screenExtent, dpi, symbol, result, false ) )
  {
    return;
  }
  QPoint renderPos = symbol.points.front() + result.offset + constState()->userOffset;
  if ( !isMultiPoint() )
  {
    // Draw line from visual reference point to actual refrence point
    context.painter()->drawLine( symbol.points.front(), symbol.points.front() + constState()->userOffset );
  }
  context.painter()->drawImage( renderPos, result.graphic );
}

bool KadasMilxItem::startPart( const QgsPointXY &firstPoint, const QgsMapSettings &mapSettings )
{
  if ( mMssString.isEmpty() )
  {
    return false;
  }
  state()->drawStatus = State::Drawing;
  state()->points.append( firstPoint );
  state()->pressedPoints = 1;

  updateSymbol( mapSettings );

  update();
  return state()->pressedPoints < mMinNPoints || mHasVariablePoints;
}

bool KadasMilxItem::startPart( const AttribValues &values, const QgsMapSettings &mapSettings )
{
  return startPart( QgsPointXY( values[AttrX], values[AttrY] ), mapSettings );
}

void KadasMilxItem::setCurrentPoint( const QgsPointXY &p, const QgsMapSettings &mapSettings )
{
  QRect screenRect = computeScreenExtent( mapSettings.extent(), mapSettings.mapToPixel() );
  int dpi = mapSettings.outputDpi();
  KadasMilxClient::NPointSymbol symbol = toSymbol( mapSettings.mapToPixel(), mapSettings.destinationCrs() );
  QgsCoordinateTransform crst( crs(), mapSettings.destinationCrs(), QgsProject::instance()->transformContext() );
  // Last possible non-control point
  int index = constState()->points.size() - 1;
  for ( ; constState()->controlPoints.contains( index ); --index );

  QPoint screenPoint = mapSettings.mapToPixel().transform( crst.transform( p ) ).toQPointF().toPoint();
  KadasMilxClient::NPointSymbolGraphic result;
  if ( KadasMilxClient::movePoint( screenRect, dpi, symbol, index, screenPoint, result ) )
  {
    updateSymbolPoints( mapSettings, result );
  }

  update();
}

void KadasMilxItem::setCurrentAttributes( const AttribValues &values, const QgsMapSettings &mapSettings )
{
  setCurrentPoint( QgsPointXY( values[AttrX], values[AttrY] ), mapSettings );
}

bool KadasMilxItem::continuePart( const QgsMapSettings &mapSettings )
{
  // Only actually add a new point if more than the minimum number have been specified
  // The server automatically adds points up to the minimum number
  ++state()->pressedPoints;

  if ( state()->pressedPoints >= mMinNPoints && mHasVariablePoints )
  {
    QRect screenRect = computeScreenExtent( mapSettings.extent(), mapSettings.mapToPixel() );
    int dpi = mapSettings.outputDpi();
    KadasMilxClient::NPointSymbol symbol = toSymbol( mapSettings.mapToPixel(), mapSettings.destinationCrs() );
    QgsCoordinateTransform crst( crs(), mapSettings.destinationCrs(), QgsProject::instance()->transformContext() );
    // Last possible non-control point
    int index = constState()->points.size() - 1;
    for ( ; constState()->controlPoints.contains( index ); --index );

    QPoint screenPoint = mapSettings.mapToPixel().transform( crst.transform( constState()->points[index] ) ).toQPointF().toPoint();
    KadasMilxClient::NPointSymbolGraphic result;
    if ( KadasMilxClient::appendPoint( screenRect, dpi, symbol, screenPoint, result ) )
    {
      updateSymbolPoints( mapSettings, result );
    }

    update();
  }
  return state()->pressedPoints < mMinNPoints || mHasVariablePoints;
}

void KadasMilxItem::endPart()
{
  state()->drawStatus = State::Finished;
}

KadasMapItem::AttribDefs KadasMilxItem::drawAttribs() const
{
  // TODO Other attributes
  AttribDefs attributes;
  attributes.insert( AttrX, NumericAttribute{"x", NumericAttribute::XCooAttr} );
  attributes.insert( AttrY, NumericAttribute{"y", NumericAttribute::YCooAttr} );
  return attributes;
}

KadasMapItem::AttribValues KadasMilxItem::drawAttribsFromPosition( const QgsPointXY &pos ) const
{
  AttribValues values;
  values.insert( AttrX, pos.x() );
  values.insert( AttrY, pos.y() );
  return values;
}

QgsPointXY KadasMilxItem::positionFromDrawAttribs( const AttribValues &values ) const
{
  return QgsPointXY( values[AttrX], values[AttrY] );
}

KadasMapItem::EditContext KadasMilxItem::getEditContext( const QgsPointXY &pos, const QgsMapSettings &mapSettings ) const
{
  QgsCoordinateTransform crst( mCrs, mapSettings.destinationCrs(), mapSettings.transformContext() );
  QgsPointXY canvasPos = mapSettings.mapToPixel().transform( crst.transform( pos ) );
  for ( int iPoint = 0, nPoints = constState()->points.size(); iPoint < nPoints; ++iPoint )
  {
    QgsPointXY testPos = mapSettings.mapToPixel().transform( crst.transform( constState()->points[iPoint] ) );
    if ( canvasPos.sqrDist( testPos ) < 25 )
    {
      return EditContext( QgsVertexId( 0, 0, iPoint ), constState()->points[iPoint], drawAttribs() );
    }
  }
  return EditContext();
}

void KadasMilxItem::edit( const EditContext &context, const QgsPointXY &newPoint, const QgsMapSettings &mapSettings )
{
  QRect screenRect = computeScreenExtent( mapSettings.extent(), mapSettings.mapToPixel() );
  int dpi = mapSettings.outputDpi();
  KadasMilxClient::NPointSymbol symbol = toSymbol( mapSettings.mapToPixel(), mapSettings.destinationCrs() );
  QgsCoordinateTransform crst( crs(), mapSettings.destinationCrs(), QgsProject::instance()->transformContext() );

  QPoint screenPoint = mapSettings.mapToPixel().transform( crst.transform( newPoint ) ).toQPointF().toPoint();
  KadasMilxClient::NPointSymbolGraphic result;
  if ( KadasMilxClient::movePoint( screenRect, dpi, symbol, context.vidx.vertex, screenPoint, result ) )
  {
    updateSymbolPoints( mapSettings, result );
  }

  update();
}

void KadasMilxItem::edit( const EditContext &context, const AttribValues &values, const QgsMapSettings &mapSettings )
{
  edit( context, QgsPointXY( values[AttrX], values[AttrY] ), mapSettings );
}

KadasMapItem::AttribValues KadasMilxItem::editAttribsFromPosition( const EditContext &context, const QgsPointXY &pos ) const
{
  return drawAttribsFromPosition( pos );
}

QgsPointXY KadasMilxItem::positionFromEditAttribs( const EditContext &context, const AttribValues &values, const QgsMapSettings &mapSettings ) const
{
  return positionFromDrawAttribs( values );
}

QList<QPoint> KadasMilxItem::computeScreenPoints( const QgsMapToPixel &mapToPixel, const QgsCoordinateTransform &mapCrst ) const
{
  QList<QPoint> points;
  for ( const QgsPointXY &p : constState()->points )
  {
    points.append( mapToPixel.transform( mapCrst.transform( p ) ).toQPointF().toPoint() );
  }
  return points;
}

QList< QPair<int, double> > KadasMilxItem::computeScreenAttributes( const QgsMapToPixel &mapToPixel, const QgsCoordinateTransform &mapCrst ) const
{
  if ( constState()->attributes.isEmpty() )
  {
    return QList<QPair<int, double> >();
  }
  double m2p = metersToPixels( constState()->points.front(), mapToPixel, mapCrst );

  QList< QPair<int, double> > screenAttribs;
  for ( const QPair<int, double> &attrib : constState()->attributes )
  {
    double value = attrib.second;
    if ( attrib.first != KadasMilxClient::AttributeAttitude )
    {
      value = value * m2p;
    }
    screenAttribs.append( qMakePair( attrib.first, value ) );
  }
  return screenAttribs;
}

bool KadasMilxItem::isMultiPoint() const
{
  return constState()->points.size() > 1 || !constState()->attributes.isEmpty();
}

KadasMilxClient::NPointSymbol KadasMilxItem::toSymbol( const QgsMapToPixel &mapToPixel, const QgsCoordinateReferenceSystem &mapCrs, bool colored ) const
{
  QgsCoordinateTransform mapCrst( mCrs, mapCrs, QgsProject::instance()->transformContext() );
  QList<QPoint> points = computeScreenPoints( mapToPixel, mapCrst );
  for ( int i = 0, n = points.size(); i < n; ++i )
  {
    points[i] += constState()->userOffset;
  }
  QList< QPair<int, double> > screenAttribs = computeScreenAttributes( mapToPixel, mapCrst );
  bool finalized = constState()->drawStatus == State::DrawStatus::Finished;
  return KadasMilxClient::NPointSymbol( mMssString, points, constState()->controlPoints, screenAttribs, finalized, colored );
}

void KadasMilxItem::writeMilx( QDomDocument &doc, QDomElement &itemElement ) const
{
  QDomElement stringXmlEl = doc.createElement( "MssStringXML" );
  stringXmlEl.appendChild( doc.createTextNode( mMssString ) );
  itemElement.appendChild( stringXmlEl );

  QDomElement nameEl = doc.createElement( "Name" );
  nameEl.appendChild( doc.createTextNode( mMilitaryName ) );
  itemElement.appendChild( nameEl );

  QDomElement pointListEl = doc.createElement( "PointList" );
  itemElement.appendChild( pointListEl );

  for ( const QgsPointXY &p : constState()->points )
  {
    QDomElement pEl = doc.createElement( "Point" );
    pointListEl.appendChild( pEl );

    QDomElement pXEl = doc.createElement( "X" );
    pXEl.appendChild( doc.createTextNode( QString::number( p.x(), 'f', 6 ) ) );
    pEl.appendChild( pXEl );
    QDomElement pYEl = doc.createElement( "Y" );
    pYEl.appendChild( doc.createTextNode( QString::number( p.y(), 'f', 6 ) ) );
    pEl.appendChild( pYEl );
  }
  if ( !constState()->attributes.isEmpty() )
  {
    QDomElement attribListEl = doc.createElement( "LocationAttributeList" );
    itemElement.appendChild( attribListEl );
    for ( const QPair<int, double> &attribute : constState()->attributes )
    {
      QDomElement attrTypeEl = doc.createElement( "AttrType" );
      attrTypeEl.appendChild( doc.createTextNode( KadasMilxClient::attributeName( attribute.first ) ) );
      QDomElement attrValueEl = doc.createElement( "Value" );
      attrValueEl.appendChild( doc.createTextNode( QString::number( attribute.second ) ) );
      QDomElement attribEl = doc.createElement( "LocationAttribute" );
      attribEl.appendChild( attrTypeEl );
      attribEl.appendChild( attrValueEl );
      attribListEl.appendChild( attribEl );
    }
  }

  if ( !isMultiPoint() )
  {
    QDomElement offsetEl = doc.createElement( "Offset" );
    itemElement.appendChild( offsetEl );

    QDomElement factorXEl = doc.createElement( "FactorX" );
    factorXEl.appendChild( doc.createTextNode( QString::number( double( constState()->userOffset.x() ) / KadasMilxClient::getSymbolSize() ) ) );
    offsetEl.appendChild( factorXEl );

    QDomElement factorYEl = doc.createElement( "FactorY" );
    factorYEl.appendChild( doc.createTextNode( QString::number( -double( constState()->userOffset.y() ) / KadasMilxClient::getSymbolSize() ) ) );
    offsetEl.appendChild( factorYEl );
  }
}

KadasMilxItem *KadasMilxItem::fromMilx( const QDomElement &itemElement, const QgsCoordinateTransform &crst, int symbolSize )
{
  KadasMilxItem *item = new KadasMilxItem();

  item->mMilitaryName = itemElement.firstChildElement( "Name" ).text();
  item->mMssString = itemElement.firstChildElement( "MssStringXML" ).text();

  bool isCorridor = itemElement.firstChildElement( "IsMIPCorridorPointList" ).text().toInt();

  QDomNodeList pointEls = itemElement.firstChildElement( "PointList" ).elementsByTagName( "Point" );
  for ( int iPoint = 0, nPoints = pointEls.count(); iPoint < nPoints; ++iPoint )
  {
    QDomElement pointEl = pointEls.at( iPoint ).toElement();
    double x = pointEl.firstChildElement( "X" ).text().toDouble();
    double y = pointEl.firstChildElement( "Y" ).text().toDouble();
    item->state()->points.append( crst.transform( QgsPoint( x, y ) ) );
  }
  QDomNodeList attribEls = itemElement.firstChildElement( "LocationAttributeList" ).elementsByTagName( "LocationAttribute" );
  for ( int iAttr = 0, nAttrs = attribEls.count(); iAttr < nAttrs; ++iAttr )
  {
    QDomElement attribEl = attribEls.at( iAttr ).toElement();
    item->state()->attributes.append( qMakePair( KadasMilxClient::attributeIdx( attribEl.firstChildElement( "AttrType" ).text() ), attribEl.firstChildElement( "Value" ).text().toDouble() ) );
  }
  double offsetX = itemElement.firstChildElement( "Offset" ).firstChildElement( "FactorX" ).text().toDouble() * symbolSize;
  double offsetY = -1. * ( itemElement.firstChildElement( "Offset" ).firstChildElement( "FactorY" ).text().toDouble() * symbolSize );

  item->state()->userOffset = QPoint( offsetX, offsetY );

  if ( item->state()->points.size() > 1 )
  {
    if ( isCorridor )
    {
      const QList<QgsPointXY> &points = item->state()->points;

      // Do some fake geo -> screen transform, since here we have no idea about screen coordinates
      double scale = 100000.;
      QPoint origin = QPoint( points[0].x() * scale, points[0].y() * scale );
      QList<QPoint> screenPoints = QList<QPoint>() << QPoint( 0, 0 );
      for ( int i = 1, n = points.size(); i < n; ++i )
      {
        screenPoints.append( QPoint( points[i].x() * scale, points[i].y() * scale ) - origin );
      }
      QList< QPair<int, double> > screenAttributes;
      if ( !item->state()->attributes.isEmpty() )
      {
        QgsDistanceArea da;
        da.setSourceCrs( QgsCoordinateReferenceSystem( "EPSG:4326" ), QgsProject::instance()->transformContext() );
        da.setEllipsoid( "WGS84" );
        QgsPointXY otherPoint( points[0].x() + 0.001, points[0].y() );
        QPointF otherScreenPoint = QPointF( otherPoint.x() * scale, otherPoint.y() * scale ) - origin;
        double ellipsoidDist = da.measureLine( points[0], otherPoint ) * QgsUnitTypes::fromUnitToUnitFactor( da.lengthUnits(), QgsUnitTypes::DistanceMeters );
        double screenDist = QVector2D( screenPoints[0] - otherScreenPoint ).length();
        for ( const QPair<int, double> &attrib : item->state()->attributes )
        {
          double value = attrib.second;
          if ( attrib.first != KadasMilxClient::AttributeAttitude )
          {
            value = value / ellipsoidDist * screenDist;
          }
          screenAttributes.append( qMakePair( attrib.first, value ) );
        }
        item->state()->attributes.clear();
      }
      if ( KadasMilxClient::getControlPoints( item->mMssString, screenPoints, screenAttributes, item->state()->controlPoints, isCorridor ) )
      {
        item->state()->points.clear();
        for ( const QPoint &screenPoint : screenPoints )
        {
          double x = ( origin.x() + screenPoint.x() ) / scale;
          double y = ( origin.y() + screenPoint.y() ) / scale;
          item->state()->points.append( QgsPointXY( x, y ) );
        }
      }
    }
    else
    {
      KadasMilxClient::getControlPointIndices( item->mMssString, item->state()->points.count(), item->state()->controlPoints );
    }
  }

  if ( item->mMilitaryName.isEmpty() )
  {
    KadasMilxClient::getMilitaryName( item->mMssString, item->mMilitaryName );
  }

  item->state()->drawStatus = State::DrawStatus::Finished;

  return item;
}

void KadasMilxItem::posPointNodeRenderer( QPainter *painter, const QgsPointXY &screenPoint, int nodeSize )
{
  painter->setPen( QPen( Qt::black, 1 ) );
  painter->setBrush( Qt::yellow );
  painter->drawEllipse( screenPoint.x() - 0.5 * nodeSize, screenPoint.y() - 0.5 * nodeSize, nodeSize, nodeSize );
}

void KadasMilxItem::ctrlPointNodeRenderer( QPainter *painter, const QgsPointXY &screenPoint, int nodeSize )
{
  painter->setPen( QPen( Qt::black, 1 ) );
  painter->setBrush( Qt::red );
  painter->drawEllipse( screenPoint.x() - 0.5 * nodeSize, screenPoint.y() - 0.5 * nodeSize, nodeSize, nodeSize );
}

QRect KadasMilxItem::computeScreenExtent( const QgsRectangle &mapExtent, const QgsMapToPixel &mapToPixel )
{
  QPoint topLeft = mapToPixel.transform( mapExtent.xMinimum(), mapExtent.yMinimum() ).toQPointF().toPoint();
  QPoint topRight = mapToPixel.transform( mapExtent.xMaximum(), mapExtent.yMinimum() ).toQPointF().toPoint();
  QPoint bottomLeft = mapToPixel.transform( mapExtent.xMinimum(), mapExtent.yMaximum() ).toQPointF().toPoint();
  QPoint bottomRight = mapToPixel.transform( mapExtent.xMaximum(), mapExtent.yMaximum() ).toQPointF().toPoint();
  int xMin = qMin( qMin( topLeft.x(), topRight.x() ), qMin( bottomLeft.x(), bottomRight.x() ) );
  int xMax = qMax( qMax( topLeft.x(), topRight.x() ), qMax( bottomLeft.x(), bottomRight.x() ) );
  int yMin = qMin( qMin( topLeft.y(), topRight.y() ), qMin( bottomLeft.y(), bottomRight.y() ) );
  int yMax = qMax( qMax( topLeft.y(), topRight.y() ), qMax( bottomLeft.y(), bottomRight.y() ) );
  return QRect( xMin, yMin, xMax - xMin, yMax - yMin ).normalized();
}

double KadasMilxItem::metersToPixels( const QgsPointXY &refPoint, const QgsMapToPixel &mapToPixel, const QgsCoordinateTransform &mapCrst ) const
{
  QgsDistanceArea da;
  da.setSourceCrs( QgsCoordinateReferenceSystem( "EPSG:4326" ), QgsProject::instance()->transformContext() );
  da.setEllipsoid( "WGS84" );

  QgsPointXY point = constState()->points.front();
  QPointF screenPoint = mapToPixel.transform( mapCrst.transform( point ) ).toQPointF();
  QgsPointXY otherPoint( point.x() + 0.001, point.y() );
  QPointF otherScreenPoint = mapToPixel.transform( mapCrst.transform( otherPoint ) ).toQPointF();
  double ellipsoidDist = da.measureLine( point, otherPoint ) * QgsUnitTypes::fromUnitToUnitFactor( da.lengthUnits(), QgsUnitTypes::DistanceMeters );
  double screenDist = QVector2D( screenPoint - otherScreenPoint ).length();
  return screenDist / ellipsoidDist;
}

void KadasMilxItem::updateSymbol( const QgsMapSettings &mapSettings )
{
  KadasMilxClient::NPointSymbol symbol = toSymbol( mapSettings.mapToPixel(), mapSettings.destinationCrs() );
  KadasMilxClient::NPointSymbolGraphic result;
  QRect screenExtent = computeScreenExtent( mapSettings.extent(), mapSettings.mapToPixel() );
  int dpi = mapSettings.outputDpi();
  KadasMilxClient::updateSymbol( screenExtent, dpi, symbol, result, true );
  updateSymbolPoints( mapSettings, result );
}

void KadasMilxItem::updateSymbolPoints( const QgsMapSettings &mapSettings, const KadasMilxClient::NPointSymbolGraphic &result )
{
  QgsCoordinateTransform mapCrst( crs(), mapSettings.destinationCrs(), QgsProject::instance()->transformContext() );
  state()->points.clear();
  for ( const QPoint &screenPoint : result.adjustedPoints )
  {
    state()->points.append( mapCrst.transform( mapSettings.mapToPixel().toMapCoordinates( screenPoint ), QgsCoordinateTransform::ReverseTransform ) );
  }

  state()->controlPoints = result.controlPoints;

  state()->attributes.clear();
  double m2p = metersToPixels( state()->points.first(), mapSettings.mapToPixel(), mapCrst );
  for ( const QPair<int, double> &attrib : result.attributes )
  {
    double value = attrib.second;
    if ( attrib.first != KadasMilxClient::AttributeAttitude )
    {
      value /= m2p;
    }
    state()->attributes.append( qMakePair( attrib.first, value ) );
  }

  state()->attributePoints.clear();
  for ( const QPair<int, QPoint> &attribPoint : result.attributePoints )
  {
    state()->attributePoints.append( qMakePair( attribPoint.first, mapCrst.transform( mapSettings.mapToPixel().toMapCoordinates( attribPoint.second ), QgsCoordinateTransform::ReverseTransform ) ) );
  }

  QRect pointBounds( result.adjustedPoints.front(), result.adjustedPoints.front() );
  for ( int i = 1, n = result.adjustedPoints.size(); i < n; ++i )
  {
    const QPoint &p = result.adjustedPoints[i];
    pointBounds.setLeft( qMin( pointBounds.left(), p.x() ) );
    pointBounds.setRight( qMax( pointBounds.right(), p.x() ) );
    pointBounds.setTop( qMin( pointBounds.top(), p.y() ) );
    pointBounds.setBottom( qMax( pointBounds.bottom(), p.y() ) );
  }

  QPoint offset = result.adjustedPoints.front() + result.offset;
  QRect symbolBounds( offset.x(), offset.y(), result.graphic.width(), result.graphic.height() );
  mMargin.setLeft( pointBounds.left() - symbolBounds.left() );
  mMargin.setTop( pointBounds.top() - symbolBounds.top() );
  mMargin.setRight( symbolBounds.right() - pointBounds.right() );
  mMargin.setBottom( symbolBounds.bottom() - pointBounds.bottom() );
}
