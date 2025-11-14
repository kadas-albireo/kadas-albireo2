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

#include <qgis/qgssettingsentryimpl.h>

#include "kadas/core/kadassettingstree.h"
#include "kadas/gui/mapitems/kadasgpxwaypointitem.h"
#include "kadas/gui/mapitemeditors/kadasgpxwaypointeditor.h"


const QgsSettingsEntryInteger *KadasGpxWaypointEditor::settingsGpxWaypointSize = new QgsSettingsEntryInteger( QStringLiteral( "waypoint_size" ), KadasSettingsTree::sTreeGpx, 2, QStringLiteral( "Waypoint size." ) );
const QgsSettingsEntryColor *KadasGpxWaypointEditor::settingsGpxWaypointColor = new QgsSettingsEntryColor( QStringLiteral( "waypoint_color" ), KadasSettingsTree::sTreeGpx, QColor( 255, 255, 0, 255 ), QStringLiteral( "Waypoint color." ) );
const QgsSettingsEntryString *KadasGpxWaypointEditor::settingsGpxWaypointLabelFont = new QgsSettingsEntryString( QStringLiteral( "waypoint_label_font" ), KadasSettingsTree::sTreeGpx, QString(), QStringLiteral( "Waypoint label font." ) );
const QgsSettingsEntryColor *KadasGpxWaypointEditor::settingsGpxWaypointLabelColor = new QgsSettingsEntryColor( QStringLiteral( "waypoint_label_color" ), KadasSettingsTree::sTreeGpx, QColor( 255, 255, 0, 255 ), QStringLiteral( "Waypoint label color." ) );

KadasGpxWaypointEditor::KadasGpxWaypointEditor( KadasMapItem *item )
  : KadasMapItemEditor( item )
{
  mUi.setupUi( this );

  connect( mUi.mLineEditName, &QLineEdit::textChanged, this, &KadasGpxWaypointEditor::syncWidgetToItem );

  mUi.mSpinBoxSize->setRange( 1, 100 );
  mUi.mSpinBoxSize->setValue( settingsGpxWaypointSize->value() );
  connect( mUi.mSpinBoxSize, qOverload<int>( &QSpinBox::valueChanged ), this, &KadasGpxWaypointEditor::saveSize );

  mUi.mToolButtonColor->setAllowOpacity( true );
  mUi.mToolButtonColor->setShowNoColor( true );
  mUi.mToolButtonColor->setColor( settingsGpxWaypointColor->value() );
  connect( mUi.mToolButtonColor, &QgsColorButton::colorChanged, this, &KadasGpxWaypointEditor::saveColor );

  QFont font;
  font.fromString( settingsGpxWaypointLabelFont->value() );
  mUi.mFontComboBox->setCurrentFont( font );
  mUi.mSpinBoxLabelSize->setValue( font.pointSize() );
  mUi.mPushButtonBold->setChecked( font.bold() );
  mUi.mPushButtonItalic->setChecked( font.italic() );
  connect( mUi.mFontComboBox, &QFontComboBox::currentFontChanged, this, &KadasGpxWaypointEditor::saveLabelFont );
  connect( mUi.mSpinBoxLabelSize, qOverload<int>( &QSpinBox::valueChanged ), this, &KadasGpxWaypointEditor::fontSizeChanged );
  connect( mUi.mPushButtonBold, &QPushButton::toggled, this, &KadasGpxWaypointEditor::saveLabelFont );
  connect( mUi.mPushButtonItalic, &QPushButton::toggled, this, &KadasGpxWaypointEditor::saveLabelFont );

  mUi.mToolButtonLabelColor->setAllowOpacity( true );
  mUi.mToolButtonLabelColor->setShowNoColor( true );
  mUi.mToolButtonLabelColor->setColor( settingsGpxWaypointLabelColor->value() );
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
  mUi.mSpinBoxSize->setValue( waypointItem->strokeWidth() );
  mUi.mSpinBoxSize->blockSignals( false );

  mUi.mToolButtonColor->blockSignals( true );
  mUi.mToolButtonColor->setColor( waypointItem->color() );
  mUi.mToolButtonColor->blockSignals( false );

  mUi.mLineEditName->blockSignals( true );
  mUi.mLineEditName->setText( waypointItem->name() );
  mUi.mLineEditName->blockSignals( false );

  mUi.mFontComboBox->blockSignals( true );
  QFont fontFamily;
  fontFamily.setFamily( waypointItem->labelFont().family() );
  mUi.mFontComboBox->setCurrentFont( fontFamily );
  mUi.mFontComboBox->blockSignals( false );

  mUi.mSpinBoxLabelSize->blockSignals( true );
  mUi.mSpinBoxLabelSize->setValue( waypointItem->labelFont().pointSize() );
  mUi.mSpinBoxLabelSize->blockSignals( false );

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

  waypointItem->setColor( color );
  waypointItem->setStrokeWidth( size );
  waypointItem->setStrokeColor( color );
  waypointItem->setIconSize( 10 + 2 * size );

  waypointItem->setName( mUi.mLineEditName->text() );

  waypointItem->setLabelFont( currentFont() );
  waypointItem->setLabelColor( mUi.mToolButtonLabelColor->color() );
}

QFont KadasGpxWaypointEditor::currentFont() const
{
  QFont font = mUi.mFontComboBox->currentFont();
  font.setBold( mUi.mPushButtonBold->isChecked() );
  font.setItalic( mUi.mPushButtonItalic->isChecked() );
  font.setPointSize( mUi.mSpinBoxLabelSize->value() );
  return font;
}

void KadasGpxWaypointEditor::saveColor()
{
  settingsGpxWaypointColor->setValue( mUi.mToolButtonColor->color() );
  emit styleChanged();
}

void KadasGpxWaypointEditor::saveSize()
{
  settingsGpxWaypointSize->setValue( mUi.mSpinBoxSize->value() );
  emit styleChanged();
}

void KadasGpxWaypointEditor::saveLabelFont()
{
  settingsGpxWaypointLabelFont->setValue( currentFont().toString() );
  emit styleChanged();
}

void KadasGpxWaypointEditor::saveLabelColor()
{
  settingsGpxWaypointLabelColor->setValue( mUi.mToolButtonLabelColor->color() );
  emit styleChanged();
}

void KadasGpxWaypointEditor::fontSizeChanged( int size )
{
  Q_UNUSED( size )
  saveLabelFont();
}
