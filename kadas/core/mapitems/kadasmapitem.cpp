/***************************************************************************
    kadasmapitem.cpp
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

#include <kadas/core/mapitems/kadasmapitem.h>


KadasMapItem::KadasMapItem(const QgsCoordinateReferenceSystem &crs, QObject* parent)
  : QObject(parent), mCrs(crs)
{
  double dMin = std::numeric_limits<double>::min();
  double dMax = std::numeric_limits<double>::max();
  mAttributes.append(NumericAttribute{"x", dMin, dMax, 0, [](const QgsPointXY& pos) { return pos.x(); }});
  mAttributes.append(NumericAttribute{"y", dMin, dMax, 0, [](const QgsPointXY& pos) { return pos.y(); }});
}

KadasMapItem::~KadasMapItem()
{
  emit aboutToBeDestroyed();
}

void KadasMapItem::setTranslationOffset(double dx, double dy)
{
  mTranslationOffset = QPointF(dx, dy);
  emit changed();
}

void KadasMapItem::setState(KadasStateStack::State* state)
{
  *mState = *state;
  recomputeDerived();
}

void KadasMapItem::reset()
{
  delete mState;
  mState = createEmptyState();
  recomputeDerived();
}
