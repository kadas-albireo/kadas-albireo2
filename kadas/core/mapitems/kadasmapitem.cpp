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
}

KadasMapItem::~KadasMapItem()
{
  emit aboutToBeDestroyed();
}

void KadasMapItem::setSelected(bool selected)
{
  mSelected = selected;
  emit changed();
}

void KadasMapItem::setZIndex(int zIndex)
{
  mZIndex = zIndex;
  emit changed();
}

void KadasMapItem::setState(const State* state)
{
  mState->assign(state);
  recomputeDerived();
}

void KadasMapItem::clear()
{
  delete mState;
  mState = createEmptyState();
  recomputeDerived();
}
