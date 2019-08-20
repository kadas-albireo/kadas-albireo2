/***************************************************************************
    kadassymbolattributeseditor.cpp
    -------------------------------
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

#include <kadas/gui/mapitems/kadassymbolitem.h>
#include <kadas/gui/mapitemeditors/kadassymbolattributeseditor.h>


KadasSymbolAttributesEditor::KadasSymbolAttributesEditor ( KadasMapItem* item )
  : KadasMapItemEditor ( item )
{
  mUi.setupUi ( this );
  connect ( mUi.mLineEditName, &QLineEdit::textChanged, this, &KadasSymbolAttributesEditor::syncWidgetToItem );
  connect ( mUi.mTextEditRemarks, &QPlainTextEdit::textChanged, this, &KadasSymbolAttributesEditor::syncWidgetToItem );
  connect ( item, &KadasMapItem::changed, this, &KadasSymbolAttributesEditor::adjustVisiblity );
  setEnabled ( false );
}

void KadasSymbolAttributesEditor::adjustVisiblity()
{
  setEnabled ( mItem->constState()->drawStatus != KadasMapItem::State::Empty );
}

void KadasSymbolAttributesEditor::reset()
{
  mUi.mLineEditName->setText ( "" );
  mUi.mTextEditRemarks->setPlainText ( "" );
}

void KadasSymbolAttributesEditor::syncItemToWidget()
{
  KadasSymbolItem* symbolItem = dynamic_cast<KadasSymbolItem*> ( mItem );
  if ( !symbolItem ) {
    return;
  }
  mUi.mLineEditName->blockSignals ( true );
  mUi.mLineEditName->setText ( symbolItem->name() );
  mUi.mLineEditName->blockSignals ( false );
  mUi.mTextEditRemarks->blockSignals ( true );
  mUi.mTextEditRemarks->setPlainText ( symbolItem->remarks() );
  mUi.mTextEditRemarks->blockSignals ( false );
}

void KadasSymbolAttributesEditor::syncWidgetToItem()
{
  KadasSymbolItem* symbolItem = dynamic_cast<KadasSymbolItem*> ( mItem );
  if ( !symbolItem ) {
    return;
  }
  symbolItem->setName ( mUi.mLineEditName->text() );
  symbolItem->setRemarks ( mUi.mTextEditRemarks->toPlainText() );
}
