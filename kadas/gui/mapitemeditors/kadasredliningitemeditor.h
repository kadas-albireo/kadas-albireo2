/***************************************************************************
    kadasredliningitemeditor.h
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

#ifndef KADASREDLININGITEMEDITOR_H
#define KADASREDLININGITEMEDITOR_H

#include <kadas/core/mapitems/kadasmapitem.h>

#include <kadas/gui/kadas_gui.h>
#include <kadas/gui/ui_kadasredliningitemeditor.h>

class KADAS_GUI_EXPORT KadasRedliningItemEditor : public KadasMapItemEditor
{
  Q_OBJECT

public:
  static KadasMapItemEditor* factory(KadasMapItem* item) {
    return new KadasRedliningItemEditor(item);
  }

  KadasRedliningItemEditor(KadasMapItem* item);
  ~KadasRedliningItemEditor();

  void syncItemToWidget() override;
  void syncWidgetToItem() override;

private:
  Ui::KadasRedliningItemEditorBase mUi;

  static QIcon createOutlineStyleIcon( Qt::PenStyle style );
  static QIcon createFillStyleIcon( Qt::BrushStyle style );

signals:
  void styleChanged();

private slots:
  void saveColor();
  void saveOutlineWidth();
  void saveStyle();
  void toggleItemMeasurements(bool enabled);
};

#endif // KADASREDLININGITEMEDITOR_H
