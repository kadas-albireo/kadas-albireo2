/***************************************************************************
    kadasitemlayerproperties.cpp
    ----------------------------
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

#include <qgis/qgsrendererpropertiesdialog.h>
#include <qgis/qgsstyle.h>
#include <qgis/qgssymbolwidgetcontext.h>

#include <kadas/gui/kadasitemlayer.h>
#include <kadas/app/kadasitemlayerproperties.h>

KadasItemLayerProperties::KadasItemLayerProperties( KadasItemLayer *layer, QWidget *parent )
  : KadasLayerPropertiesDialog( layer, parent )
{
  // Just to host the globe properties page
}
