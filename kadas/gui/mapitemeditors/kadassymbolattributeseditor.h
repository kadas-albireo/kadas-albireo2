/***************************************************************************
    kadassymbolattributeseditor.h
    -----------------------------
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

#ifndef KADASSYMBOLATTRIBUTESEDITOR_H
#define KADASSYMBOLATTRIBUTESEDITOR_H

#include <kadas/core/mapitems/kadasmapitem.h>

#include <kadas/gui/kadas_gui.h>
#include <kadas/gui/ui_kadassymbolattributeseditor.h>

class KADAS_GUI_EXPORT KadasSymbolAttributesEditor : public KadasMapItemEditor
{
  Q_OBJECT

public:
  static KadasMapItemEditor* factory ( KadasMapItem* item )
  {
    return new KadasSymbolAttributesEditor ( item );
  }

  KadasSymbolAttributesEditor ( KadasMapItem* item );

  void reset() override;
  void syncItemToWidget() override;
  void syncWidgetToItem() override;

private:
  Ui::KadasSymbolAttributesEditorBase mUi;

  void adjustVisiblity();
};

#endif // KADASSYMBOLATTRIBUTESEDITOR_H
