/***************************************************************************
    kadasgpxwaypointeditor.cpp
    --------------------------
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

#include "kadas/gui/mapitems/kadasgpxwaypointitem.h"
#include "kadas/gui/mapitemeditors/kadasgpxwaypointeditor.h"

KadasGpxWaypointEditor::KadasGpxWaypointEditor( KadasMapItem *item )
  : KadasMapItemEditor( item )
{
  mUi.setupUi( this );

  connect( mUi.mLineEditName, &QLineEdit::textChanged, this, &KadasGpxWaypointEditor::syncWidgetToItem );

  mUi.mSpinBoxSize->setRange( 1, 100 );
  mUi.mSpinBoxSize->setValue( QgsSettings().value( "/gpx/waypoint_size", 2 ).toInt() );
  connect( mUi.mSpinBoxSize, qOverload<int> ( &QSpinBox::valueChanged ), this, &KadasGpxWaypointEditor::saveSize );

  mUi.mToolButtonColor->setAllowOpacity( true );
  mUi.mToolButtonColor->setShowNoColor( true );
  mUi.mToolButtonColor->setProperty( "settings_key", "outline_color" );
  QColor initialOutlineColor = QgsSymbolLayerUtils::decodeColor( QgsSettings().value( "/gpx/waypoint_color", "255,255,0,255" ).toString() );
  mUi.mToolButtonColor->setColor( initialOutlineColor );
  connect( mUi.mToolButtonColor, &QgsColorButton::colorChanged, this, &KadasGpxWaypointEditor::saveColor );

  connect( this, &KadasGpxWaypointEditor::styleChanged, this, &KadasGpxWaypointEditor::syncWidgetToItem );

}

void KadasGpxWaypointEditor::syncItemToWidget()
{
  KadasGpxWaypointItem *waypointItem = dynamic_cast<KadasGpxWaypointItem *>( mItem );
  if ( !waypointItem )
  {
    return;
  }

  mUi.mSpinBoxSize->blockSignals( true );
  mUi.mSpinBoxSize->setValue( waypointItem->outline().width() );
  mUi.mSpinBoxSize->blockSignals( false );

  mUi.mToolButtonColor->blockSignals( true );
  mUi.mToolButtonColor->setColor( waypointItem->fill().color() );
  mUi.mToolButtonColor->blockSignals( false );

  mUi.mLineEditName->blockSignals( true );
  mUi.mLineEditName->setText( waypointItem->name() );
  mUi.mLineEditName->blockSignals( false );
}

void KadasGpxWaypointEditor::syncWidgetToItem()
{
  KadasGpxWaypointItem *waypointItem = dynamic_cast<KadasGpxWaypointItem *>( mItem );
  if ( !waypointItem )
  {
    return;
  }

  int size = mUi.mSpinBoxSize->value();
  QColor color = mUi.mToolButtonColor->color();

  waypointItem->setOutline( QPen( color, size ) );
  waypointItem->setFill( QBrush( color ) );

  waypointItem->setIconSize( 10 + 2 * size );
  waypointItem->setIconOutline( QPen( Qt::black, size / 2 ) );
  waypointItem->setIconFill( QBrush( color ) );

  waypointItem->setName( mUi.mLineEditName->text() );
}

void KadasGpxWaypointEditor::saveColor()
{
  QgsColorButton *btn = qobject_cast<QgsColorButton *> ( QObject::sender() );
  QgsSettings().setValue( "/gpx/waypoint_color", QgsSymbolLayerUtils::encodeColor( btn->color() ) );
  emit styleChanged();
}

void KadasGpxWaypointEditor::saveSize()
{
  QgsSettings().setValue( "/gpx/waypoint_size", mUi.mSpinBoxSize->value() );
  emit styleChanged();
}
