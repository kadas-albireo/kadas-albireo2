/***************************************************************************
    kadasredliningtexteditor.h
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

#ifndef KADASREDLININGTEXTEDITOR_H
#define KADASREDLININGTEXTEDITOR_H

#include <kadas/core/mapitems/kadasmapitem.h>

#include <kadas/gui/kadas_gui.h>
#include <kadas/gui/ui_kadasredliningtexteditor.h>

class KADAS_GUI_EXPORT KadasRedliningTextEditor : public KadasMapItemEditor
{
  Q_OBJECT

public:
  static KadasMapItemEditor* factory ( KadasMapItem* item )
  {
    return new KadasRedliningTextEditor ( item );
  }

  KadasRedliningTextEditor ( KadasMapItem* item );

  void syncItemToWidget() override;
  void syncWidgetToItem() override;

private:
  Ui::KadasRedliningTextEditorBase mUi;

  QFont currentFont() const;

signals:
  void styleChanged();

private slots:
  void saveColor();
  void saveFont();
  void saveText();
};

#endif // KADASREDLININGITEMEDITOR_H
