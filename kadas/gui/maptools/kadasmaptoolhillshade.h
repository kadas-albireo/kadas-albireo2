/***************************************************************************
    kadasmaptoolhillshade.h
    -----------------------
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

#ifndef KADASMAPTOOLHILLSHADE_H
#define KADASMAPTOOLHILLSHADE_H

#include "kadas/gui/kadas_gui.h"
#include "kadas/gui/maptools/kadasmaptoolcreateitem.h"

class KADAS_GUI_EXPORT KadasMapToolHillshade : public KadasMapToolCreateItem
{
    Q_OBJECT
  public:
    KadasMapToolHillshade( QgsMapCanvas *mapCanvas );
    void compute( const QgsRectangle &extent, const QgsCoordinateReferenceSystem &crs );

  private slots:
    void drawFinished();

    KadasMapToolCreateItem::ItemFactory itemFactory( const QgsMapCanvas *canvas ) const;
};

#endif // KADASMAPTOOLHILLSHADE_H
