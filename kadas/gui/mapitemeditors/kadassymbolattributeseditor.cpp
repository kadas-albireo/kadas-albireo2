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

#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>

#include "kadas/gui/kadasrichtexteditor.h"
#include "kadas/gui/mapitems/kadassymbolitem.h"
#include "kadas/gui/mapitemeditors/kadassymbolattributeseditor.h"
KadasSymbolAttributesEditor::KadasSymbolAttributesEditor( KadasMapItem *item )
  : KadasMapItemEditor( item )
{
  mLineEditName = new QLineEdit();
  mTextEditRemarks = new KadasRichTextEditor();
  mEditorToolbar = new KadasRichTextEditorToolBar( mTextEditRemarks );

  QGridLayout *layout = new QGridLayout();
  layout->addWidget( new QLabel( tr( "Name:" ) ), 0, 0, 1, 1 );
  layout->addWidget( mLineEditName, 0, 1, 1, 1 );
  layout->addWidget( new QLabel( tr( "Remarks:" ) ), 1, 0, 1, 1 );
  layout->addWidget( mEditorToolbar, 1, 1, 1, 1 );
  layout->addWidget( mTextEditRemarks, 2, 1, 1, 1 );
  setLayout( layout );

  connect( mLineEditName, &QLineEdit::textChanged, this, &KadasSymbolAttributesEditor::syncWidgetToItem );
  connect( mTextEditRemarks, &QTextEdit::textChanged, this, &KadasSymbolAttributesEditor::syncWidgetToItem );
  connect( mItem, &KadasMapItem::changed, this, &KadasSymbolAttributesEditor::adjustVisiblity );
  setEnabled( false );
}

void KadasSymbolAttributesEditor::adjustVisiblity()
{
  setEnabled( mItem && mItem->constState()->drawStatus != KadasMapItem::State::DrawStatus::Empty );
}

void KadasSymbolAttributesEditor::setItem( KadasMapItem *item )
{
  if ( mItem )
  {
    disconnect( mItem, &KadasMapItem::changed, this, &KadasSymbolAttributesEditor::adjustVisiblity );
  }
  KadasMapItemEditor::setItem( item );
  if ( mItem )
  {
    connect( mItem, &KadasMapItem::changed, this, &KadasSymbolAttributesEditor::adjustVisiblity );
  }
}

void KadasSymbolAttributesEditor::reset()
{
  mLineEditName->setText( "" );
  mTextEditRemarks->setHtml( "" );
}

void KadasSymbolAttributesEditor::syncItemToWidget()
{
  KadasSymbolItem *symbolItem = dynamic_cast<KadasSymbolItem *>( mItem );
  if ( !symbolItem )
  {
    return;
  }
  mLineEditName->blockSignals( true );
  mLineEditName->setText( symbolItem->name() );
  mLineEditName->blockSignals( false );
  mTextEditRemarks->blockSignals( true );
  mTextEditRemarks->setHtml( symbolItem->remarks() );
  mTextEditRemarks->blockSignals( false );
}

void KadasSymbolAttributesEditor::syncWidgetToItem()
{
  KadasSymbolItem *symbolItem = dynamic_cast<KadasSymbolItem *>( mItem );
  if ( !symbolItem )
  {
    return;
  }
  symbolItem->setName( mLineEditName->text() );
  symbolItem->setRemarks( mTextEditRemarks->toHtml() );
}
