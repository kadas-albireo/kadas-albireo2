/***************************************************************************
    kadasgpxrouteeditor.cpp
    -----------------------
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

#include <qgis/qgssettings.h>
#include <qgis/qgssymbollayerutils.h>

#include "kadas/gui/mapitems/kadasgpxrouteitem.h"
#include "kadas/gui/mapitemeditors/kadasgpxrouteeditor.h"

KadasGpxRouteEditor::KadasGpxRouteEditor( KadasMapItem *item )
  : KadasMapItemEditor( item )
{
  mUi.setupUi( this );

  connect( mUi.mLineEditName, &QLineEdit::textChanged, this, &KadasGpxRouteEditor::syncWidgetToItem );

  mUi.mLineEditNumber->setValidator( new QIntValidator( 0, std::numeric_limits<int>::max() ) );
  connect( mUi.mLineEditNumber, &QLineEdit::textChanged, this, &KadasGpxRouteEditor::syncWidgetToItem );

  mUi.mSpinBoxSize->setRange( 1, 100 );
  mUi.mSpinBoxSize->setValue( QgsSettings().value( "/gpx/route_size", 2 ).toInt() );
  connect( mUi.mSpinBoxSize, qOverload<int> ( &QSpinBox::valueChanged ), this, &KadasGpxRouteEditor::saveSize );

  mUi.mToolButtonColor->setAllowOpacity( true );
  mUi.mToolButtonColor->setShowNoColor( true );
  mUi.mToolButtonColor->setProperty( "settings_key", "outline_color" );
  QColor initialOutlineColor = QgsSymbolLayerUtils::decodeColor( QgsSettings().value( "/gpx/route_color", "255,255,0,255" ).toString() );
  mUi.mToolButtonColor->setColor( initialOutlineColor );
  connect( mUi.mToolButtonColor, &QgsColorButton::colorChanged, this, &KadasGpxRouteEditor::saveColor );

  connect( this, &KadasGpxRouteEditor::styleChanged, this, &KadasGpxRouteEditor::syncWidgetToItem );

  toggleItemMeasurements( true );
}

void KadasGpxRouteEditor::setItem( KadasMapItem *item )
{
  toggleItemMeasurements( false );
  KadasMapItemEditor::setItem( item );
  toggleItemMeasurements( true );
}

void KadasGpxRouteEditor::syncItemToWidget()
{
  KadasGpxRouteItem *routeItem = dynamic_cast<KadasGpxRouteItem *>( mItem );
  if ( !routeItem )
  {
    return;
  }

  mUi.mSpinBoxSize->blockSignals( true );
  mUi.mSpinBoxSize->setValue( routeItem->outline().width() );
  mUi.mSpinBoxSize->blockSignals( false );

  mUi.mToolButtonColor->blockSignals( true );
  mUi.mToolButtonColor->setColor( routeItem->outline().color() );
  mUi.mToolButtonColor->blockSignals( false );

  mUi.mLineEditName->blockSignals( true );
  mUi.mLineEditName->setText( routeItem->name() );
  mUi.mLineEditName->blockSignals( false );

  mUi.mLineEditNumber->blockSignals( true );
  mUi.mLineEditNumber->setText( routeItem->number() );
  mUi.mLineEditNumber->blockSignals( false );
}

void KadasGpxRouteEditor::syncWidgetToItem()
{
  KadasGpxRouteItem *routeItem = dynamic_cast<KadasGpxRouteItem *>( mItem );
  if ( !routeItem )
  {
    return;
  }

  int outlineWidth = mUi.mSpinBoxSize->value();
  QColor outlineColor = mUi.mToolButtonColor->color();

  routeItem->setOutline( QPen( outlineColor, outlineWidth ) );
  routeItem->setFill( QBrush( outlineColor ) );
  routeItem->setName( mUi.mLineEditName->text() );
  routeItem->setNumber( mUi.mLineEditNumber->text() );
}

KadasGpxRouteEditor::~KadasGpxRouteEditor()
{
  toggleItemMeasurements( false );
}

void KadasGpxRouteEditor::toggleItemMeasurements( bool enabled )
{
  KadasGpxRouteItem *routeItem = qobject_cast<KadasGpxRouteItem *> ( mItem );
  if ( !routeItem )
  {
    return;
  }
  routeItem->setMeasurementsEnabled( enabled );
}

void KadasGpxRouteEditor::saveColor()
{
  QgsColorButton *btn = qobject_cast<QgsColorButton *> ( QObject::sender() );
  QgsSettings().setValue( "/gpx/route_color", QgsSymbolLayerUtils::encodeColor( btn->color() ) );
  emit styleChanged();
}

void KadasGpxRouteEditor::saveSize()
{
  QgsSettings().setValue( "/gpx/route_size", mUi.mSpinBoxSize->value() );
  emit styleChanged();
}
