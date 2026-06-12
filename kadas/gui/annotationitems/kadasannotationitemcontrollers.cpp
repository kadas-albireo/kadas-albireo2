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

#include <qgis/qgsannotationitemregistry.h>
#include <qgis/qgsapplication.h>

#include "kadas/gui/annotationitems/kadasannotationcontrollerregistry.h"
#include "kadas/gui/annotationitems/kadasannotationitemcontroller.h"
#include "kadas/gui/annotationitems/kadasannotationitemcontrollers.h"
#include "kadas/gui/annotationitems/kadascircleannotationcontroller.h"
#include "kadas/gui/annotationitems/kadascircleannotationitem.h"
#include "kadas/gui/annotationitems/kadascoordcrossannotationcontroller.h"
#include "kadas/gui/annotationitems/kadascoordcrossannotationitem.h"
#include "kadas/gui/annotationitems/kadasgpxrouteannotationcontroller.h"
#include "kadas/gui/annotationitems/kadasgpxrouteannotationitem.h"
#include "kadas/gui/annotationitems/kadasgpxwaypointannotationcontroller.h"
#include "kadas/gui/annotationitems/kadasgpxwaypointannotationitem.h"
#include "kadas/gui/annotationitems/kadaslineannotationcontroller.h"
#include "kadas/gui/annotationitems/kadasmarkerannotationcontroller.h"
#include "kadas/gui/annotationitems/kadasmilxannotationcontroller.h"
#include "kadas/gui/annotationitems/kadasmilxannotationitem.h"
#include "kadas/gui/annotationitems/kadaspictureannotationcontroller.h"
#include "kadas/gui/annotationitems/kadaspinannotationcontroller.h"
#include "kadas/gui/annotationitems/kadaspinannotationitem.h"
#include "kadas/gui/annotationitems/kadaspointtextannotationcontroller.h"
#include "kadas/gui/annotationitems/kadaspolygonannotationcontroller.h"
#include "kadas/gui/annotationitems/kadasrectangleannotationcontroller.h"
#include "kadas/gui/annotationitems/kadasrectangleannotationitem.h"


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
  registry->addController( new KadasCoordCrossAnnotationController() );
  registry->addController( new KadasGpxWaypointAnnotationController() );
  registry->addController( new KadasGpxRouteAnnotationController() );
  registry->addController( new KadasMilxAnnotationController() );

  // Register Kadas-internal annotation item types with the QGIS annotation
  // item registry so they round-trip through QgsAnnotationLayer XML.
  QgsAnnotationItemRegistry *itemRegistry = QgsApplication::annotationItemRegistry();
  const auto registerType = []( QgsAnnotationItemRegistry *r, const QString &type, const QString &visible, const QString &visiblePlural, const QgsAnnotationItemCreateFunc &create ) {
    if ( !r->itemMetadata( type ) )
    {
      r->addItemType( new QgsAnnotationItemMetadata( type, visible, visiblePlural, create ) );
    }
  };
  registerType( itemRegistry, KadasCircleAnnotationItem::itemTypeId(), QObject::tr( "Kadas Circle" ), QObject::tr( "Kadas Circles" ), [] { return new KadasCircleAnnotationItem(); } );
  registerType( itemRegistry, KadasRectangleAnnotationItem::itemTypeId(), QObject::tr( "Kadas Rectangle" ), QObject::tr( "Kadas Rectangles" ), [] { return new KadasRectangleAnnotationItem(); } );
  registerType( itemRegistry, KadasPinAnnotationItem::itemTypeId(), QObject::tr( "Kadas Pin" ), QObject::tr( "Kadas Pins" ), [] { return new KadasPinAnnotationItem(); } );
  registerType( itemRegistry, KadasCoordCrossAnnotationItem::itemTypeId(), QObject::tr( "Coordinate Cross" ), QObject::tr( "Coordinate Crosses" ), [] { return new KadasCoordCrossAnnotationItem(); } );
  registerType( itemRegistry, KadasGpxWaypointAnnotationItem::itemTypeId(), QObject::tr( "GPX Waypoint" ), QObject::tr( "GPX Waypoints" ), [] { return new KadasGpxWaypointAnnotationItem(); } );
  registerType( itemRegistry, KadasGpxRouteAnnotationItem::itemTypeId(), QObject::tr( "GPX Route" ), QObject::tr( "GPX Routes" ), [] { return new KadasGpxRouteAnnotationItem(); } );
  registerType( itemRegistry, KadasMilxAnnotationItem::itemTypeId(), QObject::tr( "MilX Symbol" ), QObject::tr( "MilX Symbols" ), [] { return new KadasMilxAnnotationItem(); } );
}
