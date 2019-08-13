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

QList<QgsPointXY> KadasTextItem::rotatedCornerPoints() const
{
  QFontMetrics metrics(mFont);
  double halfW = 0.5 * metrics.width(mText);
  double halfH = 0.5 * metrics.height();
  double dx1 = - halfW;
  double dx2 = + halfW;
  double dy1 = - halfH;
  double dy2 = + halfH;

  double cosa = qCos(mAngle / 180 * M_PI);
  double sina = qSin(mAngle / 180 * M_PI);
  QgsPoint p1(cosa * dx1 - sina * dy1, sina * dx1 + cosa * dy1);
  QgsPoint p2(cosa * dx2 - sina * dy1, sina * dx2 + cosa * dy1);
  QgsPoint p3(cosa * dx2 - sina * dy2, sina * dx2 + cosa * dy2);
  QgsPoint p4(cosa * dx1 - sina * dy2, sina * dx1 + cosa * dy2);

  return QList<QgsPointXY>() << p1 << p2 << p3 << p4;
}

QRect KadasTextItem::margin() const
{
  QList<QgsPointXY> points = rotatedCornerPoints();
  int maxW = qMax(qMax(qAbs(points[0].x()), qAbs(points[1].x())), qMax(qAbs(points[2].x()), qAbs(points[3].x()))) + 1;
  int maxH = qMax(qMax(qAbs(points[0].y()), qAbs(points[1].y())), qMax(qAbs(points[2].y()), qAbs(points[3].y()))) + 1;
  return QRect(maxW, maxH, maxW, maxH);
}

QList<QgsPointXY> KadasTextItem::nodes(const QgsMapSettings& settings) const
{
  QList<QgsPointXY> points = rotatedCornerPoints();
  double mup = settings.mapUnitsPerPixel();
  double x = state()->pos.x();
  double y = state()->pos.y();
  QgsPointXY p1(x + points[0].x() * mup, y + points[0].y() * mup);
  QgsPointXY p2(x + points[1].x() * mup, y + points[1].y() * mup);
  QgsPointXY p3(x + points[2].x() * mup, y + points[2].y() * mup);
  QgsPointXY p4(x + points[3].x() * mup, y + points[3].y() * mup);
  return QList<QgsPointXY>() << p1 << p2 << p3 << p4;
}

bool KadasTextItem::intersects(const QgsRectangle& rect, const QgsMapSettings &settings) const
{
  if(mText.isEmpty()) {
    return false;
  }

  QgsRectangle r = QgsCoordinateTransform(settings.destinationCrs(), crs(), QgsProject::instance()).transform(rect);
  QList<QgsPointXY> points = nodes(settings);
  QgsPolygon imageRect;
  imageRect.setExteriorRing( new QgsLineString( QgsPointSequence() << QgsPoint(points[0]) << QgsPoint(points[1]) << QgsPoint(points[2]) << QgsPoint(points[3]) << QgsPoint(points[0]) ) );

  QgsGeometryEngine* geomEngine = QgsGeometry::createGeometryEngine( &imageRect );
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
  if(intersects(QgsRectangle(pos, pos), mapSettings)) {
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
