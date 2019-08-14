/***************************************************************************
    kadasimageitem.cpp
    ------------------
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

#include <QSvgRenderer>

#include <qgis/qgsgeometryengine.h>
#include <qgis/qgslinestring.h>
#include <qgis/qgsmapsettings.h>
#include <qgis/qgspolygon.h>
#include <qgis/qgsproject.h>

#include <kadas/core/mapitems/kadasimageitem.h>


KadasImageItem::KadasImageItem(const QgsCoordinateReferenceSystem &crs, QObject* parent)
  : KadasMapItem(crs, parent)
{
  clear();
}

void KadasImageItem::setFilePath(const QString &path, double anchorX, double anchorY)
{
  mFilePath = path;
  mAnchorX = anchorX;
  mAnchorY = anchorY;
  QSvgRenderer renderer(mFilePath);
  state()->size = renderer.viewBox().size();
  emit changed();
}

QgsRectangle KadasImageItem::boundingBox() const
{
  return QgsRectangle(state()->pos, state()->pos);
}

QList<QgsPointXY> KadasImageItem::rotatedCornerPoints() const
{
  double dx1 = - mAnchorX * state()->size.width();
  double dx2 = + (1. - mAnchorX) * state()->size.width();
  double dy1 = - (1. - mAnchorY) * state()->size.height();
  double dy2 = + mAnchorY * state()->size.height();

  double cosa = qCos(state()->angle / 180 * M_PI);
  double sina = qSin(state()->angle / 180 * M_PI);
  QgsPoint p1(cosa * dx1 - sina * dy1, sina * dx1 + cosa * dy1);
  QgsPoint p2(cosa * dx2 - sina * dy1, sina * dx2 + cosa * dy1);
  QgsPoint p3(cosa * dx2 - sina * dy2, sina * dx2 + cosa * dy2);
  QgsPoint p4(cosa * dx1 - sina * dy2, sina * dx1 + cosa * dy2);

  return QList<QgsPointXY>() << p1 << p2 << p3 << p4;
}

QRect KadasImageItem::margin() const
{
  QList<QgsPointXY> points = rotatedCornerPoints();
  int maxW = qMax(qMax(qAbs(points[0].x()), qAbs(points[1].x())), qMax(qAbs(points[2].x()), qAbs(points[3].x()))) + 1;
  int maxH = qMax(qMax(qAbs(points[0].y()), qAbs(points[1].y())), qMax(qAbs(points[2].y()), qAbs(points[3].y()))) + 1;
  return QRect(maxW, maxH, maxW, maxH);
}

QList<QgsPointXY> KadasImageItem::nodes(const QgsMapSettings& settings) const
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

bool KadasImageItem::intersects(const QgsRectangle& rect, const QgsMapSettings &settings) const
{
  if(mFilePath.isEmpty()) {
    return false;
  }

  QgsRectangle r = QgsCoordinateTransform(settings.destinationCrs(), crs(), QgsProject::instance()).transform(rect);
  QList<QgsPointXY> points = nodes(settings);
  QgsPolygon imageRect;
  imageRect.setExteriorRing( new QgsLineString( QgsPointSequence() << QgsPoint(points[0]) << QgsPoint(points[1]) << QgsPoint(points[2]) << QgsPoint(points[3]) << QgsPoint(points[0]) ) );

  QgsPolygon filterRect;
  QgsLineString* exterior = new QgsLineString();
  exterior->setPoints( QgsPointSequence()
                       << QgsPoint( r.xMinimum(), r.yMinimum() )
                       << QgsPoint( r.xMaximum(), r.yMinimum() )
                       << QgsPoint( r.xMaximum(), r.yMaximum() )
                       << QgsPoint( r.xMinimum(), r.yMaximum() )
                       << QgsPoint( r.xMinimum(), r.yMinimum() ) );
  filterRect.setExteriorRing( exterior );

  QgsGeometryEngine* geomEngine = QgsGeometry::createGeometryEngine( &imageRect );
  bool intersects = geomEngine->intersects(&filterRect);
  delete geomEngine;
  return intersects;
}

void KadasImageItem::render( QgsRenderContext &context ) const
{
  if(state()->drawStatus == State::Empty) {
    return;
  }

  QgsPoint pos = QgsPoint(state()->pos);
  pos.transform( context.coordinateTransform() );
  pos.transform( context.mapToPixel().transform() );

  QSvgRenderer svgRenderer(mFilePath);

  //keep width/height ratio of svg
  QRect viewBox = svgRenderer.viewBox();
  if ( viewBox.isValid() )
  {
    double scale = 1.0; // TODO
    QSize frameSize = viewBox.size() * scale;
    double widthRatio = frameSize.width() / viewBox.width();
    double heightRatio = frameSize.height() / viewBox.height();
    double renderWidth = 0;
    double renderHeight = 0;
    if ( widthRatio <= heightRatio )
    {
      renderWidth = frameSize.width();
      renderHeight = viewBox.height() * frameSize.width() / viewBox.width();
    }
    else
    {
      renderHeight = frameSize.height();
      renderWidth = viewBox.width() * frameSize.height() / viewBox.height();
    }
    context.painter()->save();
    context.painter()->scale( scale, scale );
    context.painter()->translate(pos.x(), pos.y());
    context.painter()->rotate( -state()->angle );
    context.painter()->translate(- mAnchorX * state()->size.width(), - mAnchorY * state()->size.height());
    svgRenderer.render( context.painter(), QRectF( 0, 0, renderWidth, renderHeight ) );
    context.painter()->restore();
  }
}

bool KadasImageItem::startPart(const QgsPointXY& firstPoint)
{
  state()->drawStatus = State::Drawing;
  state()->pos = firstPoint;
  recomputeDerived();
  return false;
}

bool KadasImageItem::startPart(const AttribValues& values)
{
  return startPart(QgsPointXY(values[AttrX], values[AttrY]));
}

void KadasImageItem::setCurrentPoint(const QgsPointXY& p, const QgsMapSettings* mapSettings)
{
  // Do nothing
}

void KadasImageItem::setCurrentAttributes(const AttribValues& values)
{
  // Do nothing
}

bool KadasImageItem::continuePart()
{
  // No further action allowed
  return false;
}

void KadasImageItem::endPart()
{
  state()->drawStatus = State::Finished;
}

KadasMapItem::AttribDefs KadasImageItem::drawAttribs() const
{
  AttribDefs attributes;
  attributes.insert(AttrX, NumericAttribute{"x"});
  attributes.insert(AttrY, NumericAttribute{"y"});
  return attributes;
}

KadasMapItem::AttribValues KadasImageItem::drawAttribsFromPosition(const QgsPointXY& pos) const
{
  AttribValues values;
  values.insert(AttrX, pos.x());
  values.insert(AttrY, pos.y());
  return values;
}

QgsPointXY KadasImageItem::positionFromDrawAttribs(const AttribValues& values) const
{
  return QgsPointXY(values[AttrX], values[AttrY]);
}

KadasMapItem::EditContext KadasImageItem::getEditContext(const QgsPointXY& pos, const QgsMapSettings& mapSettings) const
{
  if(intersects(QgsRectangle(pos, pos), mapSettings)) {
    return EditContext(QgsVertexId(0, 0, 0), state()->pos, drawAttribs());
  }
  return EditContext();
}

void KadasImageItem::edit(const EditContext& context, const QgsPointXY& newPoint, const QgsMapSettings* mapSettings)
{
  if(context.vidx.isValid()) {
    state()->pos = newPoint;
    recomputeDerived();
  }
}

void KadasImageItem::edit(const EditContext& context, const AttribValues& values)
{
  edit(context, QgsPointXY(values[AttrX], values[AttrY]));
}

KadasMapItem::AttribValues KadasImageItem::editAttribsFromPosition(const EditContext& context, const QgsPointXY& pos) const
{
  return drawAttribsFromPosition(pos);
}

QgsPointXY KadasImageItem::positionFromEditAttribs(const EditContext& context, const AttribValues& values) const
{
  return positionFromDrawAttribs(values);
}

void KadasImageItem::recomputeDerived()
{
  emit changed();
}
