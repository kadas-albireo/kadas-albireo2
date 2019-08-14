/***************************************************************************
    kadasimageattributeseditor.cpp
    ------------------------------
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

#include <kadas/core/mapitems/kadasimageitem.h>

#include <kadas/gui/mapitemeditors/kadasimageattributeseditor.h>


KadasImageAttributesEditor::KadasImageAttributesEditor(KadasMapItem* item)
  : KadasMapItemEditor(item)
{
  mUi.setupUi(this);
  connect(mUi.mLineEditName, &QLineEdit::textChanged, this, &KadasImageAttributesEditor::syncWidgetToItem);
  connect(mUi.mTextEditRemarks, &QPlainTextEdit::textChanged, this, &KadasImageAttributesEditor::syncWidgetToItem);
}

void KadasImageAttributesEditor::reset()
{
  mUi.mLineEditName->setText("");
  mUi.mTextEditRemarks->setPlainText("");
}

void KadasImageAttributesEditor::syncItemToWidget()
{
  KadasImageItem* imageItem = dynamic_cast<KadasImageItem*>(mItem);
  if(!imageItem) {
    return;
  }
  mUi.mLineEditName->blockSignals(true);
  mUi.mLineEditName->setText(imageItem->name());
  mUi.mLineEditName->blockSignals(false);
  mUi.mTextEditRemarks->blockSignals(true);
  mUi.mTextEditRemarks->setPlainText(imageItem->remarks());
  mUi.mTextEditRemarks->blockSignals(false);
}

void KadasImageAttributesEditor::syncWidgetToItem()
{
  KadasImageItem* imageItem = dynamic_cast<KadasImageItem*>(mItem);
  if(!imageItem) {
    return;
  }
  imageItem->setName(mUi.mLineEditName->text());
  imageItem->setRemarks(mUi.mTextEditRemarks->toPlainText());
}
