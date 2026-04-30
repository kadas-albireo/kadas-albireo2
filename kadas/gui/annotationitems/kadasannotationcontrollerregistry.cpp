/***************************************************************************
    kadasannotationcontrollerregistry.cpp
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

#include "kadas/gui/annotationitems/kadasannotationcontrollerregistry.h"
#include "kadas/gui/annotationitems/kadasannotationitemcontroller.h"


KadasAnnotationControllerRegistry *KadasAnnotationControllerRegistry::instance()
{
  static KadasAnnotationControllerRegistry sInstance;
  return &sInstance;
}

KadasAnnotationControllerRegistry::~KadasAnnotationControllerRegistry()
{
  qDeleteAll( mControllers );
}

void KadasAnnotationControllerRegistry::addController( KadasAnnotationItemController *controller )
{
  if ( !controller )
    return;
  const QString typeId = controller->itemType();
  if ( typeId.isEmpty() )
  {
    delete controller;
    return;
  }
  if ( KadasAnnotationItemController *existing = mControllers.value( typeId, nullptr ) )
  {
    if ( existing == controller )
      return;
    delete existing;
    mControllers.insert( typeId, controller );
    return;
  }
  mControllers.insert( typeId, controller );
  mOrder.append( typeId );
}

KadasAnnotationItemController *KadasAnnotationControllerRegistry::controllerFor( const QString &typeId ) const
{
  return mControllers.value( typeId, nullptr );
}

QStringList KadasAnnotationControllerRegistry::registeredTypes() const
{
  return mOrder;
}
