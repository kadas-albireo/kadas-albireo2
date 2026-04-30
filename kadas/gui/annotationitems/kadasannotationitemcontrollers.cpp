/***************************************************************************
    kadasannotationitemcontrollers.cpp
    ----------------------------------
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
#include "kadas/gui/annotationitems/kadasannotationitemcontrollers.h"
#include "kadas/gui/annotationitems/kadascircleannotationcontroller.h"
#include "kadas/gui/annotationitems/kadasgpxrouteannotationcontroller.h"
#include "kadas/gui/annotationitems/kadasgpxwaypointannotationcontroller.h"
#include "kadas/gui/annotationitems/kadaslineannotationcontroller.h"
#include "kadas/gui/annotationitems/kadasmarkerannotationcontroller.h"
#include "kadas/gui/annotationitems/kadaspictureannotationcontroller.h"
#include "kadas/gui/annotationitems/kadaspinannotationcontroller.h"
#include "kadas/gui/annotationitems/kadaspointtextannotationcontroller.h"
#include "kadas/gui/annotationitems/kadaspolygonannotationcontroller.h"
#include "kadas/gui/annotationitems/kadasrectangleannotationcontroller.h"


void KadasAnnotationItemControllers::registerBuiltins()
{
  static bool sRegistered = false;
  if ( sRegistered )
    return;
  sRegistered = true;

  KadasAnnotationControllerRegistry *registry = KadasAnnotationControllerRegistry::instance();

  // Stock-type controllers (QGIS annotation item type ids).
  registry->addController( new KadasMarkerAnnotationController() );
  registry->addController( new KadasLineAnnotationController() );
  registry->addController( new KadasPolygonAnnotationController() );
  registry->addController( new KadasPictureAnnotationController() );
  registry->addController( new KadasPointTextAnnotationController() );

  // Kadas-internal type controllers (kadas:* type ids).
  registry->addController( new KadasCircleAnnotationController() );
  registry->addController( new KadasRectangleAnnotationController() );
  registry->addController( new KadasPinAnnotationController() );
  registry->addController( new KadasGpxWaypointAnnotationController() );
  registry->addController( new KadasGpxRouteAnnotationController() );
}
