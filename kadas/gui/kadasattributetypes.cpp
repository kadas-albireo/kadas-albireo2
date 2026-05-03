/***************************************************************************
    kadasattributetypes.cpp
    -----------------------
    copyright            : (C) 2026 by Denis Rouzaud
    email                : denis@opengis.ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "kadas/gui/kadasattributetypes.h"

#include <qgis/qgis.h>
#include <qgis/qgscoordinatereferencesystem.h>
#include <qgis/qgsmapsettings.h>
#include <qgis/qgsunittypes.h>


int KadasNumericAttribute::precision( const QgsMapSettings &mapSettings ) const
{
  if ( decimals >= 0 )
  {
    return decimals;
  }
  if ( type == Type::TypeCoordinate )
  {
    return mapSettings.destinationCrs().mapUnits() == Qgis::DistanceUnit::Degrees ? 3 : 0;
  }
  return 0;
}

QString KadasNumericAttribute::suffix( const QgsMapSettings &mapSettings ) const
{
  switch ( type )
  {
    case Type::TypeCoordinate:
      return QString();
    case Type::TypeDistance:
      return QgsUnitTypes::toAbbreviatedString( mapSettings.destinationCrs().mapUnits() );
    case Type::TypeAngle:
      return QString( "°" );
    case Type::TypeOther:
      return QString();
  }
  return QString();
}
