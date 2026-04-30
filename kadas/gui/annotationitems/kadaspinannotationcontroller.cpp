/***************************************************************************
    kadaspinannotationcontroller.cpp
    --------------------------------
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

#include "kadas/gui/annotationitems/kadaspinannotationcontroller.h"
#include "kadas/gui/annotationitems/kadaspinannotationitem.h"


QString KadasPinAnnotationController::itemType() const
{
  return KadasPinAnnotationItem::itemTypeId();
}

QString KadasPinAnnotationController::itemName() const
{
  return QObject::tr( "Pin" );
}

QgsAnnotationItem *KadasPinAnnotationController::createItem() const
{
  return new KadasPinAnnotationItem();
}
