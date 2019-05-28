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

#include <qgis/qgsgeometryengine.h>
#include <qgis/qgslinestring.h>
#include <qgis/qgsmapsettings.h>
#include <qgis/qgspolygon.h>
#include <qgis/qgsproject.h>

#include <kadas/core/mapitems/kadastextitem.h>


KadasTextItem::KadasTextItem(const QgsCoordinateReferenceSystem &crs, QObject* parent)
  : KadasMapItem(crs, parent)
{
  clear();
}

void KadasTextItem::setText(const QString& text)
{
  mText = text;
  emit changed();
}

void KadasTextItem::setFillColor( const QColor& c )
{
  mFillColor = c;
  emit changed();
}

void KadasTextItem::setOutlineColor( const QColor& c )
{
  mOutlineColor = c;
  emit changed();
}

void KadasTextItem::setFont( const QFont& font )
{
  mFont = font;
  emit changed();
}

void KadasTextItem::setRotation(double angle)
{
  mAngle = angle;
  emit changed();
}

QgsRectangle KadasTextItem::boundingBox() const
{
  return QgsRectangle(state()->pos, state()->pos);
}

QSize KadasTextItem::margin() const
{
  QFontMetrics metrics(mFont);
  double halfW = 0.5 * metrics.width(mText);
  double halfH = 0.5 * metrics.height();
  double cosa = qCos(mAngle / 180. * M_PI);
  double sina = qSin(mAngle / 180. * M_PI);
  QPointF p1(cosa * -halfW - sina * -halfH, sina * -halfW + cosa * -halfH);
  QPointF p2(cosa * +halfW - sina * -halfH, sina * +halfW + cosa * -halfH);
  QPointF p3(cosa * +halfW - sina * +halfH, sina * +halfW + cosa * +halfH);
  QPointF p4(cosa * -halfW - sina * +halfH, sina * -halfW + cosa * +halfH);
  int maxW = qMax(qMax(qAbs(p1.x()), qAbs(p2.x())), qMax(qAbs(p3.x()), qAbs(p4.x()))) + 1;
  int maxH = qMax(qMax(qAbs(p1.y()), qAbs(p2.y())), qMax(qAbs(p3.y()), qAbs(p4.y()))) + 1;
  return QSize(maxW, maxH);
}

QList<QgsPointXY> KadasTextItem::nodes() const
{
  return QList<QgsPointXY>() << state()->pos;
}

bool KadasTextItem::intersects(const QgsRectangle& rect, const QgsMapSettings &settings) const
{
  if(mText.isEmpty()) {
    return false;
  }

  QgsRectangle r = QgsCoordinateTransform(settings.destinationCrs(), crs(), QgsProject::instance()).transform(rect);

  QFontMetrics metrics(mFont);
  double x = state()->pos.x();
  double y = state()->pos.y();
  double halfW = .5 * metrics.width(mText) * settings.mapUnitsPerPixel();
  double halfH = .5 * metrics.height() * settings.mapUnitsPerPixel();
  double cosa = qCos(mAngle / 180 * M_PI);
  double sina = qSin(mAngle / 180 * M_PI);
  QgsPoint p1(x + cosa * -halfW - sina * -halfH, y + sina * -halfW + cosa * -halfH);
  QgsPoint p2(x + cosa * +halfW - sina * -halfH, y + sina * +halfW + cosa * -halfH);
  QgsPoint p3(x + cosa * +halfW - sina * +halfH, y + sina * +halfW + cosa * +halfH);
  QgsPoint p4(x + cosa * -halfW - sina * +halfH, y + sina * -halfW + cosa * +halfH);

  QgsPolygon textRect;
  textRect.setExteriorRing( new QgsLineString( QgsPointSequence() << p1 << p2 << p3 << p4 << p1 ) );

  QgsGeometryEngine* geomEngine = QgsGeometry::createGeometryEngine( &textRect );
  QgsPoint center = QgsPoint(r.center());
  bool intersects = geomEngine->contains(&center);
  delete geomEngine;
  return intersects;
}

void KadasTextItem::render( QgsRenderContext &context ) const
{
  QgsPointXY mapPos = context.coordinateTransform().transform(state()->pos);
  QPointF pos = context.mapToPixel().transform(mapPos).toQPointF();
  QFontMetrics metrics(mFont);
  QRect bbox = metrics.boundingRect(mText);
  int baselineOffset = metrics.ascent() - 0.5 * metrics.height();

  context.painter()->save();
  context.painter()->setBrush( QBrush(mFillColor) );
  context.painter()->setPen( QPen(mOutlineColor, mFont.pointSizeF() / 15., Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin) );
  context.painter()->setFont( mFont );
  QPainterPath path;
  path.addText(-0.5 * bbox.width(), baselineOffset, mFont, mText);
  context.painter()->translate(pos);
  context.painter()->rotate(-mAngle);
  context.painter()->drawPath(path);
  context.painter()->restore();
}

bool KadasTextItem::startPart(const QgsPointXY& firstPoint)
{
  state()->drawStatus = State::Drawing;
  state()->pos = firstPoint;
  recomputeDerived();
  return false;
}

bool KadasTextItem::startPart(const AttribValues& values)
{
  return startPart(QgsPointXY(values[AttrX], values[AttrY]));
}

void KadasTextItem::setCurrentPoint(const QgsPointXY& p, const QgsMapSettings* mapSettings)
{
  // Do nothing
}

void KadasTextItem::setCurrentAttributes(const AttribValues& values)
{
  // Do nothing
}

bool KadasTextItem::continuePart()
{
  // No further action allowed
  return false;
}

void KadasTextItem::endPart()
{
  state()->drawStatus = State::Finished;
}

KadasMapItem::AttribDefs KadasTextItem::drawAttribs() const
{
  AttribDefs attributes;
  attributes.insert(AttrX, NumericAttribute{"x"});
  attributes.insert(AttrY, NumericAttribute{"y"});
  return attributes;
}

KadasMapItem::AttribValues KadasTextItem::drawAttribsFromPosition(const QgsPointXY& pos) const
{
  AttribValues values;
  values.insert(AttrX, pos.x());
  values.insert(AttrY, pos.y());
  return values;
}

QgsPointXY KadasTextItem::positionFromDrawAttribs(const AttribValues& values) const
{
  return QgsPointXY(values[AttrX], values[AttrY]);
}

KadasMapItem::EditContext KadasTextItem::getEditContext(const QgsPointXY& pos, const QgsMapSettings& mapSettings) const
{
  QgsCoordinateTransform crst(mCrs, mapSettings.destinationCrs(), mapSettings.transformContext());
  QgsPointXY canvasPos = mapSettings.mapToPixel().transform(crst.transform(pos));
  QgsPointXY testPos = mapSettings.mapToPixel().transform(crst.transform(state()->pos));
  if ( canvasPos.sqrDist(testPos) < 25 ) {
    return EditContext(QgsVertexId(0, 0, 0), state()->pos, drawAttribs());
  }
  return EditContext();
}

void KadasTextItem::edit(const EditContext& context, const QgsPointXY& newPoint, const QgsMapSettings* mapSettings)
{
  if(context.vidx.isValid()) {
    state()->pos = newPoint;
    recomputeDerived();
  }
}

void KadasTextItem::edit(const EditContext& context, const AttribValues& values)
{
  edit(context, QgsPointXY(values[AttrX], values[AttrY]));
}

KadasMapItem::AttribValues KadasTextItem::editAttribsFromPosition(const EditContext& context, const QgsPointXY& pos) const
{
  return drawAttribsFromPosition(pos);
}

QgsPointXY KadasTextItem::positionFromEditAttribs(const EditContext& context, const AttribValues& values) const
{
  return positionFromDrawAttribs(values);
}

void KadasTextItem::recomputeDerived()
{
  emit changed();
}
