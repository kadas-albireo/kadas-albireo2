/***************************************************************************
    kadasmapitemeditor.h
    --------------------
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

#ifndef KADASMAPITEMEDITOR_H
#define KADASMAPITEMEDITOR_H

#include <QWidget>

#include <kadas/gui/kadas_gui.h>


class KadasMapItem;

class KADAS_GUI_EXPORT KadasMapItemEditor : public QWidget
{
    Q_OBJECT

  public:
    enum EditorType {CreateItemEditor, EditItemEditor};

    KadasMapItemEditor( KadasMapItem *item, QWidget *parent = nullptr ) : QWidget( parent ), mItem( item ) {}

    virtual void setItem( KadasMapItem *item ) { mItem = item; }

  public slots:
    virtual void reset() {}
    virtual void syncItemToWidget() = 0;
    virtual void syncWidgetToItem() = 0;

  protected:
    KadasMapItem *mItem;
};

#endif // KADASMAPITEMEDITOR_H
