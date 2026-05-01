/***************************************************************************
    kadasgpxrouteannotationcontroller.cpp
    -------------------------------------
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

#include <QObject>

#include "kadas/gui/annotationitems/kadasannotationzindex.h"
#include "kadas/gui/annotationitems/kadasgpxrouteannotationcontroller.h"
#include "kadas/gui/annotationitems/kadasgpxrouteannotationitem.h"


QString KadasGpxRouteAnnotationController::itemType() const
{
  return KadasGpxRouteAnnotationItem::itemTypeId();
}

QString KadasGpxRouteAnnotationController::itemName() const
{
  return QObject::tr( "Route" );
}

QgsAnnotationItem *KadasGpxRouteAnnotationController::createItem() const
{
  auto *item = new KadasGpxRouteAnnotationItem();
  item->setZIndex( KadasAnnotationZIndex::GpxRoute );
  return item;
}
