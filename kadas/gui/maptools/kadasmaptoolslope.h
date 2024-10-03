/***************************************************************************
    kadasmaptoolslope.h
    -------------------
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

#ifndef KADASMAPTOOLSLOPE_H
#define KADASMAPTOOLSLOPE_H

#include "kadas/gui/kadas_gui.h"
#include "kadas/gui/kadasmapiteminterface.h"
#include "kadas/gui/maptools/kadasmaptoolcreateitem.h"

class KADAS_GUI_EXPORT KadasMapToolSlope : public KadasMapToolCreateItem, public KadasMapItemInterface
{
    Q_OBJECT
  public:
    KadasMapToolSlope( QgsMapCanvas *mapCanvas );
    void compute( const QgsRectangle &extent, const QgsCoordinateReferenceSystem &crs );

    KadasMapItem* createItem() const override;

  private slots:
    void drawFinished();

};

#endif // KADASMAPTOOLSLOPE_H
