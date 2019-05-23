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
#include <qgis/qgsmapsettings.h>

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

bool KadasPointItem::startPart(const AttribValues& attributeValues)
{
  QgsPoint point(attributeValues[AttrX], attributeValues[AttrY]);
  return startPart(point);
}

void KadasPointItem::setCurrentPoint(const QgsPointXY& p, const QgsMapSettings &mapSettings)
{
  // Do nothing
}

void KadasPointItem::setCurrentAttributes(const AttribValues& values)
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

KadasMapItem::AttribDefs KadasPointItem::drawAttribs() const
{
  double dMin = std::numeric_limits<double>::min();
  double dMax = std::numeric_limits<double>::max();
  AttribDefs attributes;
  attributes.insert(AttrX, NumericAttribute{"x", dMin, dMax, 0});
  attributes.insert(AttrY, NumericAttribute{"y", dMin, dMax, 0});
  return attributes;
}

KadasMapItem::AttribValues KadasPointItem::drawAttribsFromPosition(const QgsPointXY& pos) const
{
  AttribValues values;
  values.insert(AttrX, pos.x());
  values.insert(AttrY, pos.y());
  return values;
}

QgsPointXY KadasPointItem::positionFromDrawAttribs(const AttribValues& values) const
{
  return QgsPointXY(values[AttrX], values[AttrY]);
}

KadasMapItem::EditContext KadasPointItem::getEditContext(const QgsPointXY& pos, const QgsMapSettings& mapSettings) const
{
  QgsCoordinateTransform crst(mCrs, mapSettings.destinationCrs(), mapSettings.transformContext());
  QgsPointXY canvasPos = mapSettings.mapToPixel().transform(crst.transform(pos));
  for(int i = 0, n = state()->points.size(); i < n; ++i) {
    QgsPointXY testPos = mapSettings.mapToPixel().transform(crst.transform(state()->points[i]));
    if ( canvasPos.sqrDist(testPos) < 25 ) {
      return EditContext(QgsVertexId(i, 0, 0));
    }
  }
  return EditContext();
}

void KadasPointItem::edit(const EditContext& context, const QgsPointXY& newPoint, const QgsMapSettings& mapSettings)
{
  if(context.vidx.part >= 0 && context.vidx.part < state()->points.size()) {
    state()->points[context.vidx.part] = newPoint;
    recomputeDerived();
  }
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
