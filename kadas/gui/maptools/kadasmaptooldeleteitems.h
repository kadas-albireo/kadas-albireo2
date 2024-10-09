/***************************************************************************
    kadasmaptooldeleteitems.h
    -------------------------
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

#ifndef KADASMAPTOOLDELETEITEMS_H
#define KADASMAPTOOLDELETEITEMS_H

#include "kadas/gui/kadas_gui.h"
#include "kadas/gui/kadasmapiteminterface.h"
#include "kadas/gui/maptools/kadasmaptoolcreateitem.h"

class KADAS_GUI_EXPORT KadasMapToolDeleteItemsInterface : public KadasMapItemInterface
{
  public:
    KadasMapToolDeleteItemsInterface( QgsMapCanvas *mapCanvas ) : KadasMapItemInterface(), mCanvas( mapCanvas ) {}
    KadasMapItem* createItem() const override;
  private:
    QgsMapCanvas* mCanvas = nullptr;
};


class KADAS_GUI_EXPORT KadasMapToolDeleteItems : public KadasMapToolCreateItem
{
    Q_OBJECT
  public:
    KadasMapToolDeleteItems( QgsMapCanvas *mapCanvas );
    void activate() override;
    void deleteItems( const KadasMapRect &filterRect );

  private slots:
    void drawFinished();
};

#endif // KADASMAPTOOLDELETEITEMS_H
