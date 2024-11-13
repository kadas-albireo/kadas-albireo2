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
  QColor initialOutlineColor = QgsSymbolLayerUtils::decodeColor( QgsSettings().value( "/gpx/waypoint_color", "255,255,0,255" ).toString() );
  mUi.mToolButtonColor->setColor( initialOutlineColor );
  connect( mUi.mToolButtonColor, &QgsColorButton::colorChanged, this, &KadasGpxWaypointEditor::saveColor );

  QFont font;
  font.fromString( QgsSettings().value( "/gpx/waypoint_label_font", "" ).toString() );
  mUi.mFontComboBox->setCurrentFont( font );
  mUi.mSpinBoxSize->setValue( font.pointSize() );
  mUi.mPushButtonBold->setChecked( font.bold() );
  mUi.mPushButtonItalic->setChecked( font.italic() );
  connect( mUi.mFontComboBox, &QFontComboBox::currentFontChanged, this, &KadasGpxWaypointEditor::saveLabelFont );
  connect( mUi.mSpinBoxSize, qOverload<int> ( &QSpinBox::valueChanged ), this, &KadasGpxWaypointEditor::fontSizeChanged );
  connect( mUi.mPushButtonBold, &QPushButton::toggled, this, &KadasGpxWaypointEditor::saveLabelFont );
  connect( mUi.mPushButtonItalic, &QPushButton::toggled, this, &KadasGpxWaypointEditor::saveLabelFont );

  mUi.mToolButtonLabelColor->setAllowOpacity( true );
  mUi.mToolButtonLabelColor->setShowNoColor( true );
  QColor initialLabelColor = QgsSymbolLayerUtils::decodeColor( QgsSettings().value( "/gpx/waypoint_label_color", "255,255,0,255" ).toString() );
  mUi.mToolButtonLabelColor->setColor( initialLabelColor );
  connect( mUi.mToolButtonLabelColor, &QgsColorButton::colorChanged, this, &KadasGpxWaypointEditor::saveLabelColor );

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

  mUi.mFontComboBox->blockSignals( true );
  QFont fontFamily;
  fontFamily.setFamily( waypointItem->labelFont().family() );
  mUi.mFontComboBox->setCurrentFont( fontFamily );
  mUi.mFontComboBox->blockSignals( false );

  mUi.mSpinBoxSize->blockSignals( true );
  mUi.mSpinBoxSize->setValue( waypointItem->labelFont().pointSize() );
  mUi.mSpinBoxSize->blockSignals( false );

  mUi.mPushButtonBold->blockSignals( true );
  mUi.mPushButtonBold->setChecked( waypointItem->labelFont().bold() );
  mUi.mPushButtonBold->blockSignals( false );

  mUi.mPushButtonItalic->blockSignals( true );
  mUi.mPushButtonItalic->setChecked( waypointItem->labelFont().italic() );
  mUi.mPushButtonItalic->blockSignals( false );

  mUi.mToolButtonLabelColor->blockSignals( true );
  mUi.mToolButtonLabelColor->setColor( waypointItem->labelColor() );
  mUi.mToolButtonLabelColor->blockSignals( false );
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

  waypointItem->setLabelFont( currentFont() );
  waypointItem->setLabelColor( mUi.mToolButtonLabelColor->color() );
}

QFont KadasGpxWaypointEditor::currentFont() const
{
  QFont font = mUi.mFontComboBox->currentFont();
  font.setBold( mUi.mPushButtonBold->isChecked() );
  font.setItalic( mUi.mPushButtonItalic->isChecked() );
  font.setPointSize( mUi.mSpinBoxSize->value() );
  return font;
}

void KadasGpxWaypointEditor::saveColor()
{
  QgsSettings().setValue( "/gpx/waypoint_color", QgsSymbolLayerUtils::encodeColor( mUi.mToolButtonColor->color() ) );
  emit styleChanged();
}

void KadasGpxWaypointEditor::saveSize()
{
  QgsSettings().setValue( "/gpx/waypoint_size", mUi.mSpinBoxSize->value() );
  emit styleChanged();
}

void KadasGpxWaypointEditor::saveLabelFont()
{
  QgsSettings().setValue( "/gpx/waypoint_label_font", currentFont().toString() );
  emit styleChanged();
}

void KadasGpxWaypointEditor::saveLabelColor()
{
  QgsSettings().setValue( "/gpx/waypoint_label_color", QgsSymbolLayerUtils::encodeColor( mUi.mToolButtonLabelColor->color() ) );
  emit styleChanged();
}

void KadasGpxWaypointEditor::fontSizeChanged(int size)
{
  Q_UNUSED(size)
  saveLabelFont();
}
