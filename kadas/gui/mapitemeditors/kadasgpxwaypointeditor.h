/***************************************************************************
    kadasgpxwaypointeditor.h
    ------------------------
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

#ifndef KADASGPXWAYPOINTEDITOR_H
#define KADASGPXWAYPOINTEDITOR_H

#include "kadas/gui/kadas_gui.h"
#include "kadas/gui/mapitems/kadasmapitem.h"
#include "kadas/gui/mapitemeditors/kadasmapitemeditor.h"
#include "kadas/gui/ui_kadasgpxwaypointeditor.h"

class KADAS_GUI_EXPORT KadasGpxWaypointEditor : public KadasMapItemEditor
{
    Q_OBJECT

  public:
    KadasGpxWaypointEditor( KadasMapItem *item );

    void syncItemToWidget() override;
    void syncWidgetToItem() override;

  private:
    Ui::KadasGpxWaypointEditorBase mUi;

    QFont currentFont() const;

  signals:
    void styleChanged();

  private slots:
    void saveColor();
    void saveSize();
    void saveLabelFont();
    void saveLabelColor();
    void fontSizeChanged(int size);
};

#endif // KADASGPXWAYPOINTEDITOR_H
