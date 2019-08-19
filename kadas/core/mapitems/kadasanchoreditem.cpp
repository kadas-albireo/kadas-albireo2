/***************************************************************************
    kadasanchoreditem.cpp
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

#include <qgis/qgsgeometryengine.h>
#include <qgis/qgslinestring.h>
#include <qgis/qgsmapsettings.h>
#include <qgis/qgspolygon.h>
#include <qgis/qgsproject.h>

#include <kadas/core/mapitems/kadasanchoreditem.h>


KadasAnchoredItem::KadasAnchoredItem ( const QgsCoordinateReferenceSystem& crs, QObject* parent )
  : KadasMapItem ( crs, parent )
{
  clear();
}

void KadasAnchoredItem::setAnchor ( double anchorX, double anchorY )
{
  mAnchorX = anchorX;
  mAnchorY = anchorY;
}

void KadasAnchoredItem::setPosition ( const QgsPointXY& pos )
{
  state()->pos = pos;
  state()->drawStatus = State::DrawStatus::Finished;
  emit changed();
}

QgsRectangle KadasAnchoredItem::boundingBox() const
{
  return QgsRectangle ( state()->pos, state()->pos );
}

QList<QgsPointXY> KadasAnchoredItem::rotatedCornerPoints ( double angle, double mup ) const
{
  double dx1 = - mAnchorX * state()->size.width();
  double dx2 = + ( 1. - mAnchorX ) * state()->size.width();
  double dy1 = - ( 1. - mAnchorY ) * state()->size.height();
  double dy2 = + mAnchorY * state()->size.height();

  double x = state()->pos.x();
  double y = state()->pos.y();
  double cosa = qCos ( angle / 180 * M_PI );
  double sina = qSin ( angle / 180 * M_PI );
  QgsPointXY p1 ( x + ( cosa * dx1 - sina * dy1 ) * mup, y + ( sina * dx1 + cosa * dy1 ) * mup );
  QgsPointXY p2 ( x + ( cosa * dx2 - sina * dy1 ) * mup, y + ( sina * dx2 + cosa * dy1 ) * mup );
  QgsPointXY p3 ( x + ( cosa * dx2 - sina * dy2 ) * mup, y + ( sina * dx2 + cosa * dy2 ) * mup );
  QgsPointXY p4 ( x + ( cosa * dx1 - sina * dy2 ) * mup, y + ( sina * dx1 + cosa * dy2 ) * mup );

  return QList<QgsPointXY>() << p1 << p2 << p3 << p4;
}

QRect KadasAnchoredItem::margin() const
{
  QList<QgsPointXY> points = rotatedCornerPoints ( state()->angle );
  int maxW = qMax ( qMax ( qAbs ( points[0].x() ), qAbs ( points[1].x() ) ), qMax ( qAbs ( points[2].x() ), qAbs ( points[3].x() ) ) ) + 1;
  int maxH = qMax ( qMax ( qAbs ( points[0].y() ), qAbs ( points[1].y() ) ), qMax ( qAbs ( points[2].y() ), qAbs ( points[3].y() ) ) ) + 1;
  return QRect ( maxW, maxH, maxW, maxH );
}

QList<KadasMapItem::Node> KadasAnchoredItem::nodes ( const QgsMapSettings& settings ) const
{
  QList<QgsPointXY> points = rotatedCornerPoints ( state()->angle, settings.mapUnitsPerPixel() );
  QList<Node> nodes;
  nodes.append ( {points[0]} );
  nodes.append ( {points[1]} );
  nodes.append ( {points[2]} );
  nodes.append ( {points[3]} );
  nodes.append ( {QgsPointXY ( 0.5 * ( points[1].x() + points[2].x() ), 0.5 * ( points[1].y() + points[2].y() ) ), rotateNodeRenderer} );
  nodes.append ( {state()->pos, anchorNodeRenderer} );
  return nodes;
}

bool KadasAnchoredItem::intersects ( const QgsRectangle& rect, const QgsMapSettings& settings ) const
{
  if ( state()->size.isEmpty() ) {
    return false;
  }

  QList<QgsPointXY> points = rotatedCornerPoints ( state()->angle, settings.mapUnitsPerPixel() );
  QgsPolygon imageRect;
  imageRect.setExteriorRing ( new QgsLineString ( QgsPointSequence() << QgsPoint ( points[0] ) << QgsPoint ( points[1] ) << QgsPoint ( points[2] ) << QgsPoint ( points[3] ) << QgsPoint ( points[0] ) ) );

  QgsPolygon filterRect;
  QgsLineString* exterior = new QgsLineString();
  exterior->setPoints ( QgsPointSequence()
                        << QgsPoint ( rect.xMinimum(), rect.yMinimum() )
                        << QgsPoint ( rect.xMaximum(), rect.yMinimum() )
                        << QgsPoint ( rect.xMaximum(), rect.yMaximum() )
                        << QgsPoint ( rect.xMinimum(), rect.yMaximum() )
                        << QgsPoint ( rect.xMinimum(), rect.yMinimum() ) );
  filterRect.setExteriorRing ( exterior );

  QgsGeometryEngine* geomEngine = QgsGeometry::createGeometryEngine ( &imageRect );
  bool intersects = geomEngine->intersects ( &filterRect );
  delete geomEngine;
  return intersects;
}

bool KadasAnchoredItem::startPart ( const QgsPointXY& firstPoint )
{
  state()->drawStatus = State::Drawing;
  state()->pos = firstPoint;
  recomputeDerived();
  return false;
}

bool KadasAnchoredItem::startPart ( const AttribValues& values )
{
  return startPart ( QgsPointXY ( values[AttrX], values[AttrY] ) );
}

void KadasAnchoredItem::setCurrentPoint ( const QgsPointXY& p, const QgsMapSettings* mapSettings )
{
  // Do nothing
}

void KadasAnchoredItem::setCurrentAttributes ( const AttribValues& values )
{
  // Do nothing
}

bool KadasAnchoredItem::continuePart()
{
  // No further action allowed
  return false;
}

void KadasAnchoredItem::endPart()
{
  state()->drawStatus = State::Finished;
}

KadasMapItem::AttribDefs KadasAnchoredItem::drawAttribs() const
{
  AttribDefs attributes;
  attributes.insert ( AttrX, NumericAttribute{"x"} );
  attributes.insert ( AttrY, NumericAttribute{"y"} );
  return attributes;
}

KadasMapItem::AttribValues KadasAnchoredItem::drawAttribsFromPosition ( const QgsPointXY& pos ) const
{
  AttribValues values;
  values.insert ( AttrX, pos.x() );
  values.insert ( AttrY, pos.y() );
  return values;
}

QgsPointXY KadasAnchoredItem::positionFromDrawAttribs ( const AttribValues& values ) const
{
  return QgsPointXY ( values[AttrX], values[AttrY] );
}

KadasMapItem::EditContext KadasAnchoredItem::getEditContext ( const QgsPointXY& pos, const QgsMapSettings& mapSettings ) const
{
  QgsCoordinateTransform crst ( mCrs, mapSettings.destinationCrs(), mapSettings.transformContext() );
  QgsPointXY canvasPos = mapSettings.mapToPixel().transform ( crst.transform ( pos ) );
  QList<QgsPointXY> points = rotatedCornerPoints ( state()->angle, mapSettings.mapUnitsPerPixel() );
  QgsPointXY rotateHandlePos ( 0.5 * ( points[1].x() + points[2].x() ), 0.5 * ( points[1].y() + points[2].y() ) );
  QgsPointXY testPos = mapSettings.mapToPixel().transform ( crst.transform ( rotateHandlePos ) );
  if ( canvasPos.sqrDist ( testPos ) < 25 ) {
    AttribDefs attributes;
    attributes.insert ( AttrA, NumericAttribute{QString ( QChar ( 0x03B1 ) ), 0} );
    return EditContext ( QgsVertexId ( 0, 0, 1 ), rotateHandlePos, attributes );
  }
  double tol = mapSettings.mapUnitsPerPixel();
  if ( intersects ( QgsRectangle ( pos.x() - tol, pos.y() - tol, pos.x() + tol, pos.y() + tol ), mapSettings ) ) {
    return EditContext ( QgsVertexId ( 0, 0, 0 ), state()->pos, drawAttribs() );
  }
  return EditContext();
}

void KadasAnchoredItem::edit ( const EditContext& context, const QgsPointXY& newPoint, const QgsMapSettings* mapSettings )
{
  if ( context.vidx.isValid() ) {
    if ( context.vidx.vertex == 1 ) {
      QgsVector delta = newPoint - state()->pos;
      // Rotate handle is in the middle of the right edge
      QgsPointXY dir ( state()->size.width() - mAnchorX * state()->size.width(), 0.5 * state()->size.height() - mAnchorY * state()->size.height() );
      double offset = qAtan2 ( dir.y(), dir.x() );
      double angle = ( qAtan2 ( delta.y(), delta.x() ) + offset ) / M_PI * 180.;
      // If less than 5 deg from quarter, snap to quarter
      if ( qAbs ( angle - qRound ( angle / 90. ) * 90. ) < 5 ) {
        angle = qRound ( angle / 90. ) * 90.;
      }
      state()->angle = angle;
    } else {
      state()->pos = newPoint;
    }
    recomputeDerived();
  }
}

void KadasAnchoredItem::edit ( const EditContext& context, const AttribValues& values )
{
  if ( context.vidx.isValid() ) {
    if ( context.vidx.vertex == 1 ) {
      state()->angle = values[AttrA];
    } else {
      state()->pos = QgsPointXY ( values[AttrX], values[AttrY] );
    }
    recomputeDerived();
  }
}

KadasMapItem::AttribValues KadasAnchoredItem::editAttribsFromPosition ( const EditContext& context, const QgsPointXY& pos ) const
{
  if ( context.vidx.vertex == 1 ) {
    QgsPointXY dir ( state()->size.width() - mAnchorX * state()->size.width(), 0.5 * state()->size.height() - mAnchorY * state()->size.height() );
    double offset = qAtan2 ( dir.y(), dir.x() );
    QgsVector delta = pos - state()->pos;
    double angle = ( qAtan2 ( delta.y(), delta.x() ) + offset ) / M_PI * 180.;
    while ( angle < 0 ) {
      angle += 360;
    }
    AttribValues values;
    values.insert ( AttrA, angle );
    return values;
  } else {
    return drawAttribsFromPosition ( pos );
  }
}

QgsPointXY KadasAnchoredItem::positionFromEditAttribs ( const EditContext& context, const AttribValues& values, const QgsMapSettings& mapSettings ) const
{
  if ( context.vidx.vertex == 1 ) {
    QList<QgsPointXY> points = rotatedCornerPoints ( values[AttrA], mapSettings.mapUnitsPerPixel() );
    return QgsPointXY ( 0.5 * ( points[1].x() + points[2].x() ), 0.5 * ( points[1].y() + points[2].y() ) );
  } else {
    return positionFromDrawAttribs ( values );
  }
}

void KadasAnchoredItem::recomputeDerived()
{
  emit changed();
}

void KadasAnchoredItem::rotateNodeRenderer ( QPainter* painter, const QgsPointXY& screenPoint, int nodeSize )
{
  painter->setPen ( QPen ( Qt::red, 2 ) );
  painter->setBrush ( Qt::white );
  painter->drawEllipse ( screenPoint.x() - 0.5 * nodeSize, screenPoint.y() - 0.5 * nodeSize, nodeSize, nodeSize );
}
