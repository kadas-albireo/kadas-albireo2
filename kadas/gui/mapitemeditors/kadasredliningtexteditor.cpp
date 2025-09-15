/***************************************************************************
    kadasredliningtexteditor.cpp
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

#include <qgis/qgssettings.h>
#include <qgis/qgssymbollayerutils.h>

#include "kadas/gui/mapitems/kadastextitem.h"
#include "kadas/gui/mapitemeditors/kadasredliningtexteditor.h"


KadasRedliningTextEditor::KadasRedliningTextEditor( KadasMapItem *item )
  : KadasMapItemEditor( item )
{
  mUi.setupUi( this );

  mUi.mLineEditText->setText( QgsSettings().value( "/Redlining/text", tr( "Text" ) ).toString() );
  connect( mUi.mLineEditText, &QLineEdit::textChanged, this, &KadasRedliningTextEditor::saveText );

  QFont font;
  font.setPointSize( 20 );
  font.fromString( QgsSettings().value( "/Redlining/font", QFont().toString() ).toString() );

  QFont fontFamily;
  fontFamily.setFamily( font.family() );
  mUi.mFontComboBox->setCurrentFont( fontFamily );
  connect( mUi.mFontComboBox, &QFontComboBox::currentFontChanged, this, &KadasRedliningTextEditor::saveFont );

  mUi.mSpinBoxSize->setValue( font.pointSize() );
  connect( mUi.mSpinBoxSize, qOverload<int>( &QSpinBox::valueChanged ), this, &KadasRedliningTextEditor::saveFont );

  mUi.mPushButtonBold->setChecked( font.bold() );
  connect( mUi.mPushButtonBold, &QPushButton::toggled, this, &KadasRedliningTextEditor::saveFont );

  mUi.mPushButtonItalic->setChecked( font.italic() );
  connect( mUi.mPushButtonItalic, &QPushButton::toggled, this, &KadasRedliningTextEditor::saveFont );

  mUi.mToolButtonBorderColor->setAllowOpacity( true );
  mUi.mToolButtonBorderColor->setShowNoColor( true );
  QColor initialOutlineColor = QgsSymbolLayerUtils::decodeColor( QgsSettings().value( "/Redlining/text_outline_color", "0,0,0,0" ).toString() );
  mUi.mToolButtonBorderColor->setColor( initialOutlineColor );
  connect( mUi.mToolButtonBorderColor, &QgsColorButton::colorChanged, this, &KadasRedliningTextEditor::saveOutlineColor );

  mUi.mToolButtonFillColor->setAllowOpacity( true );
  QColor initialFillColor = QgsSymbolLayerUtils::decodeColor( QgsSettings().value( "/Redlining/text_color", "255,0,0,255" ).toString() );
  mUi.mToolButtonFillColor->setColor( initialFillColor );
  connect( mUi.mToolButtonFillColor, &QgsColorButton::colorChanged, this, &KadasRedliningTextEditor::saveFillColor );

  KadasTextItem *textItem = dynamic_cast<KadasTextItem *>( mItem );
  if ( !textItem )
  {
    return;
  }
  // TODO !!! mItemConnection = connect( textItem, &KadasTextItem::propertyChanged, this, &KadasRedliningTextEditor::syncItemToWidget );
}

void KadasRedliningTextEditor::syncItemToWidget()
{
  KadasTextItem *textItem = dynamic_cast<KadasTextItem *>( mItem );
  if ( !textItem )
  {
    return;
  }

  mUi.mLineEditText->blockSignals( true );
  mUi.mLineEditText->setText( textItem->text() );
  mUi.mLineEditText->blockSignals( false );

  mUi.mToolButtonBorderColor->blockSignals( true );
  mUi.mToolButtonBorderColor->setColor( textItem->outlineColor() );
  mUi.mToolButtonBorderColor->blockSignals( false );

  mUi.mToolButtonFillColor->blockSignals( true );
  mUi.mToolButtonFillColor->setColor( textItem->fillColor() );
  mUi.mToolButtonFillColor->blockSignals( false );

  mUi.mFontComboBox->blockSignals( true );
  QFont fontFamily;
  fontFamily.setFamily( textItem->font().family() );
  mUi.mFontComboBox->setCurrentFont( fontFamily );
  mUi.mFontComboBox->blockSignals( false );

  mUi.mSpinBoxSize->blockSignals( true );
  mUi.mSpinBoxSize->setValue( textItem->font().pointSize() );
  mUi.mSpinBoxSize->blockSignals( false );

  mUi.mPushButtonBold->blockSignals( true );
  mUi.mPushButtonBold->setChecked( textItem->font().bold() );
  mUi.mPushButtonBold->blockSignals( false );

  mUi.mPushButtonItalic->blockSignals( true );
  mUi.mPushButtonItalic->setChecked( textItem->font().italic() );
  mUi.mPushButtonItalic->blockSignals( false );
}

void KadasRedliningTextEditor::syncWidgetToItem()
{
  KadasTextItem *textItem = dynamic_cast<KadasTextItem *>( mItem );
  if ( !textItem )
  {
    return;
  }
  textItem->setText( mUi.mLineEditText->text() );
  textItem->setOutlineColor( mUi.mToolButtonBorderColor->color() );
  textItem->setFillColor( mUi.mToolButtonFillColor->color() );
  textItem->setFont( currentFont() );
}

void KadasRedliningTextEditor::setItem( KadasMapItem *item )
{
  disconnect( mItemConnection );

  KadasMapItemEditor::setItem( item );

  KadasTextItem *textItem = dynamic_cast<KadasTextItem *>( mItem );
  if ( !textItem )
  {
    return;
  }
  // TODO !!! mItemConnection = connect( textItem, &KadasTextItem::propertyChanged, this, &KadasRedliningTextEditor::syncItemToWidget );
}

QFont KadasRedliningTextEditor::currentFont() const
{
  QFont font = mUi.mFontComboBox->currentFont();
  font.setBold( mUi.mPushButtonBold->isChecked() );
  font.setItalic( mUi.mPushButtonItalic->isChecked() );
  font.setPointSize( mUi.mSpinBoxSize->value() );
  return font;
}

void KadasRedliningTextEditor::saveFillColor()
{
  QgsSettings().setValue( "text_color", QgsSymbolLayerUtils::encodeColor( mUi.mToolButtonFillColor->color() ) );

  KadasTextItem *textItem = dynamic_cast<KadasTextItem *>( mItem );
  if ( !textItem )
  {
    return;
  }
  textItem->setFillColor( mUi.mToolButtonFillColor->color() );
}

void KadasRedliningTextEditor::saveOutlineColor()
{
  QgsSettings().setValue( "text_outline_color", QgsSymbolLayerUtils::encodeColor( mUi.mToolButtonBorderColor->color() ) );

  KadasTextItem *textItem = dynamic_cast<KadasTextItem *>( mItem );
  if ( !textItem )
  {
    return;
  }
  textItem->setOutlineColor( mUi.mToolButtonBorderColor->color() );
}

void KadasRedliningTextEditor::saveFont()
{
  QgsSettings().setValue( "/Redlining/font", currentFont().toString() );

  KadasTextItem *textItem = dynamic_cast<KadasTextItem *>( mItem );
  if ( !textItem )
  {
    return;
  }
  textItem->setFont( currentFont() );
}

void KadasRedliningTextEditor::saveText()
{
  QgsSettings().setValue( "/Redlining/text", mUi.mLineEditText->text() );

  KadasTextItem *textItem = dynamic_cast<KadasTextItem *>( mItem );
  if ( !textItem )
  {
    return;
  }
  textItem->setText( mUi.mLineEditText->text() );
}
