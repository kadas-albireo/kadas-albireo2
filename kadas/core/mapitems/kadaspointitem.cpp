/***************************************************************************
    kadaspointitem.cpp
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

#include <qgis/qgspoint.h>
#include <qgis/qgsmultipoint.h>

#include <kadas/core/mapitems/kadaspointitem.h>


KadasPointItem::KadasPointItem(const QgsCoordinateReferenceSystem &crs, IconType icon, QObject* parent)
  : KadasGeometryItem(crs, parent)
{
  setIconType(icon);
  clear();
}

bool KadasPointItem::startPart(const QgsPointXY& firstPoint)
{
  state()->drawStatus = State::Drawing;
  state()->points.append(firstPoint);
  recomputeDerived();
  return false;
}

bool KadasPointItem::startPart(const QList<double>& attributeValues)
{
  QgsPoint point(attributeValues[AttrX], attributeValues[AttrY]);
  return startPart(point);
}

void KadasPointItem::setCurrentPoint(const QgsPointXY& p, const QgsMapSettings &mapSettings)
{
  // Do nothing
}

void KadasPointItem::setCurrentAttributes(const QList<double>& values)
{
  // Do nothing
}

bool KadasPointItem::continuePart()
{
  // No further action allowed
  return false;
}

void KadasPointItem::endPart()
{
  state()->drawStatus = State::Finished;
}

const QgsMultiPoint* KadasPointItem::geometry() const
{
  return static_cast<QgsMultiPoint*>(mGeometry);
}

QgsMultiPoint* KadasPointItem::geometry()
{
  return static_cast<QgsMultiPoint*>(mGeometry);
}

void KadasPointItem::recomputeDerived()
{
  QgsMultiPoint* multiGeom = new QgsMultiPoint();
  for ( const QgsPointXY& point : state()->points )
  {
    multiGeom->addGeometry(new QgsPoint(point));
  }
  setGeometry(multiGeom);
}

QList<KadasMapItem::NumericAttribute> KadasPointItem::attributes() const
{
  double dMin = std::numeric_limits<double>::min();
  double dMax = std::numeric_limits<double>::max();
  QList<KadasMapItem::NumericAttribute> attributes;
  attributes.insert(AttrX, NumericAttribute{"x", dMin, dMax, 0});
  attributes.insert(AttrY, NumericAttribute{"y", dMin, dMax, 0});
  return attributes;
}

QList<double> KadasPointItem::attributesFromPosition(const QgsPointXY& pos) const
{
  QList<double> values;
  values.insert(AttrX, pos.x());
  values.insert(AttrY, pos.y());
  return values;
}

QgsPointXY KadasPointItem::positionFromAttributes(const QList<double>& values) const
{
  return QgsPointXY(values[AttrX], values[AttrY]);
}
