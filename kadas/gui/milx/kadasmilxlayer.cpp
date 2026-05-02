/***************************************************************************
    kadasmilxlayer.cpp
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

#include "kadas/gui/milx/kadasmilxclient.h"
#include "kadas/gui/milx/kadasmilxlayer.h"

KadasMilxLayer::KadasMilxLayer( const QString &name )
  : KadasItemLayer( name, QgsCoordinateReferenceSystem( "EPSG:4326" ), layerType() )
{}

const KadasMilxSymbolSettings &KadasMilxLayer::milxSymbolSettings() const
{
  if ( mOverrideMilxSymbolSettings )
  {
    return mMilxSymbolSettings;
  }
  else
  {
    return KadasMilxClient::globalSymbolSettings();
  }
}
