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

#include <kadas/gui/kadas_gui.h>
#include <kadas/gui/maptools/kadasmaptooldrawshape.h>

class KADAS_GUI_EXPORT KadasMapToolDeleteItems : public KadasMapToolDrawRectangle
{
    Q_OBJECT
  public:
    KadasMapToolDeleteItems( QgsMapCanvas* mapCanvas );
    void activate() override;
    void deleteItems( const QgsRectangle& filterRect, const QgsCoordinateReferenceSystem& filterRectCrs );

  private slots:
    void drawFinished();
};

#endif // KADASMAPTOOLDELETEITEMS_H
