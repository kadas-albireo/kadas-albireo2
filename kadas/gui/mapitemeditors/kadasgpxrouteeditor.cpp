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

  QFont font;
  font.fromString( QgsSettings().value( "/gpx/route_label_font", "" ).toString() );
  mUi.mFontComboBox->setCurrentFont( font );
  mUi.mSpinBoxLabelSize->setValue( font.pointSize() );
  mUi.mPushButtonBold->setChecked( font.bold() );
  mUi.mPushButtonItalic->setChecked( font.italic() );
  connect( mUi.mFontComboBox, &QFontComboBox::currentFontChanged, this, &KadasGpxRouteEditor::saveLabelFont );
  connect( mUi.mSpinBoxLabelSize, qOverload<int> ( &QSpinBox::valueChanged ), this, &KadasGpxRouteEditor::fontSizeChanged );
  connect( mUi.mPushButtonBold, &QPushButton::toggled, this, &KadasGpxRouteEditor::saveLabelFont );
  connect( mUi.mPushButtonItalic, &QPushButton::toggled, this, &KadasGpxRouteEditor::saveLabelFont );

  mUi.mToolButtonLabelColor->setAllowOpacity( true );
  mUi.mToolButtonLabelColor->setShowNoColor( true );
  QColor initialLabelColor = QgsSymbolLayerUtils::decodeColor( QgsSettings().value( "/gpx/route_label_color", "255,255,0,255" ).toString() );
  mUi.mToolButtonLabelColor->setColor( initialLabelColor );
  connect( mUi.mToolButtonLabelColor, &QgsColorButton::colorChanged, this, &KadasGpxRouteEditor::saveLabelColor );

  connect( this, &KadasGpxRouteEditor::styleChanged, this, &KadasGpxRouteEditor::syncWidgetToItem );

  toggleItemMeasurements( true );
}

KadasGpxRouteEditor::~KadasGpxRouteEditor()
{
  toggleItemMeasurements( false );
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

  mUi.mFontComboBox->blockSignals( true );
  QFont fontFamily;
  fontFamily.setFamily( routeItem->labelFont().family() );
  mUi.mFontComboBox->setCurrentFont( fontFamily );
  mUi.mFontComboBox->blockSignals( false );

  mUi.mSpinBoxLabelSize->blockSignals( true );
  mUi.mSpinBoxLabelSize->setValue( routeItem->labelFont().pointSize() );
  mUi.mSpinBoxLabelSize->blockSignals( false );

  mUi.mPushButtonBold->blockSignals( true );
  mUi.mPushButtonBold->setChecked( routeItem->labelFont().bold() );
  mUi.mPushButtonBold->blockSignals( false );

  mUi.mPushButtonItalic->blockSignals( true );
  mUi.mPushButtonItalic->setChecked( routeItem->labelFont().italic() );
  mUi.mPushButtonItalic->blockSignals( false );

  mUi.mToolButtonLabelColor->blockSignals( true );
  mUi.mToolButtonLabelColor->setColor( routeItem->labelColor() );
  mUi.mToolButtonLabelColor->blockSignals( false );
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

  routeItem->setLabelFont( currentFont() );
  routeItem->setLabelColor( mUi.mToolButtonLabelColor->color() );
}

QFont KadasGpxRouteEditor::currentFont() const
{
  QFont font = mUi.mFontComboBox->currentFont();
  font.setBold( mUi.mPushButtonBold->isChecked() );
  font.setItalic( mUi.mPushButtonItalic->isChecked() );
  font.setPointSize( mUi.mSpinBoxLabelSize->value() );
  return font;
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

void KadasGpxRouteEditor::saveLabelFont()
{
  QgsSettings().setValue( "/gpx/route_label_font", currentFont().toString() );
  emit styleChanged();
}

void KadasGpxRouteEditor::saveLabelColor()
{
  QgsSettings().setValue( "/gpx/route_label_color", QgsSymbolLayerUtils::encodeColor( mUi.mToolButtonLabelColor->color() ) );
  emit styleChanged();
}

void KadasGpxRouteEditor::fontSizeChanged(int size)
{
  Q_UNUSED(size)
  saveLabelFont();
}
