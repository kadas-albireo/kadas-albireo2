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

#include <qgis/qgsannotationmarkeritem.h>
#include <qgis/qgsmarkersymbol.h>
#include <qgis/qgspoint.h>

#include "kadas/gui/annotationitems/kadasannotationzindex.h"
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
  auto *item = new KadasPinAnnotationItem();
  item->setZIndex( KadasAnnotationZIndex::Pin );
  return item;
}

QList<QgsAnnotationItem *> KadasPinAnnotationController::generateShadows( const QgsAnnotationItem *item, const KadasAnnotationItemContext &ctx ) const
{
  Q_UNUSED( ctx );
  const auto *master = static_cast<const KadasPinAnnotationItem *>( item );
  const QgsPointXY pt = master->geometry();
  auto *shadow = new QgsAnnotationMarkerItem( QgsPoint( pt.x(), pt.y() ) );
  if ( master->symbol() )
    shadow->setSymbol( master->symbol()->clone() );
  shadow->setZIndex( master->zIndex() );
  return { shadow };
}
