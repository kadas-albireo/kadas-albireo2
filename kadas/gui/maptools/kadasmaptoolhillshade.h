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

#include <qgis/qgsmaptool.h>
#include "kadas/gui/maptools/kadasshapecapturemaptool.h"

#include "kadas/gui/kadas_gui.h"

class KADAS_GUI_EXPORT KadasMapToolHillshade : public KadasShapeCaptureMapTool
{
    Q_OBJECT
  public:
    KadasMapToolHillshade( QgsMapCanvas *mapCanvas );
    void compute( const QgsRectangle &extent, const QgsCoordinateReferenceSystem &crs );

  private slots:
    void onShapeCaptured( const QgsGeometry &geometry, const QgsCoordinateReferenceSystem &crs );
};

#endif // KADASMAPTOOLHILLSHADE_H
