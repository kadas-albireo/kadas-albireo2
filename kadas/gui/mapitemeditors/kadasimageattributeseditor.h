/***************************************************************************
    kadasimageattributeseditor.h
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

#ifndef KADASIMAGEATTRIBUTESEDITOR_H
#define KADASIMAGEATTRIBUTESEDITOR_H

#include <kadas/core/mapitems/kadasmapitem.h>

#include <kadas/gui/kadas_gui.h>
#include <kadas/gui/ui_kadasimageattributeseditor.h>

class KADAS_GUI_EXPORT KadasImageAttributesEditor : public KadasMapItemEditor
{
  Q_OBJECT

public:
  static KadasMapItemEditor* factory(KadasMapItem* item) {
    return new KadasImageAttributesEditor(item);
  }

  KadasImageAttributesEditor(KadasMapItem* item);

  void reset() override;
  void syncItemToWidget() override;
  void syncWidgetToItem() override;

private:
  Ui::KadasImageAttributesEditorBase mUi;
};

#endif // KADASIMAGEATTRIBUTESEDITOR_H
