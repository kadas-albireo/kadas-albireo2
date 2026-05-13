/***************************************************************************
    kadasmaptoolheightprofile.h
    ---------------------------
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

#ifndef KADASMAPTOOLHEIGHTPROFILE_H
#define KADASMAPTOOLHEIGHTPROFILE_H

#include <QList>

#include <qgis/qgscoordinatereferencesystem.h>
#include <qgis/qgspointxy.h>

#include "kadas/gui/kadas_gui.h"
#include "kadas/gui/maptools/kadasshapecapturemaptool.h"

class QgsAbstractGeometry;
class QgsGeometry;
class QgsRubberBand;
class KadasHeightProfileDialog;

class KADAS_GUI_EXPORT KadasMapToolHeightProfile : public KadasShapeCaptureMapTool
{
    Q_OBJECT
  public:
    KadasMapToolHeightProfile( QgsMapCanvas *canvas );

    void canvasMoveEvent( QgsMapMouseEvent *e ) override;
    void canvasReleaseEvent( QgsMapMouseEvent *e ) override;
    void keyReleaseEvent( QKeyEvent *e ) override;
    void activate() override;
    void deactivate() override;

    void setGeometry( const QgsAbstractGeometry &geom, const QgsCoordinateReferenceSystem &crs );
    void setMarkerPos( double distance );

  public slots:
    void pickLine();

  private slots:
    void onShapeCaptured( const QgsGeometry &geom, const QgsCoordinateReferenceSystem &crs );
    void onCleared();

  private:
    QgsRubberBand *mPosMarker = nullptr;
    KadasHeightProfileDialog *mDialog = nullptr;
    bool mPicking = false;
    QList<QgsPointXY> mCapturedPoints;
    QgsCoordinateReferenceSystem mCapturedCrs;
};

#endif // KADASMAPTOOLHEIGHTPROFILE_H
