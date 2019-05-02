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

#include <qgis/qgsmaptool.h>

#include <kadas/gui/kadas_gui.h>

class QgsGeometry;
class QgsRubberBand;
class QgsVectorLayer;
class KadasHeightProfileDialog;
class KadasMapToolDrawPolyLine;

class KADAS_GUI_EXPORT KadasMapToolHeightProfile : public QgsMapTool
{
    Q_OBJECT
  public:

    KadasMapToolHeightProfile( QgsMapCanvas* canvas );
    ~KadasMapToolHeightProfile();

    void canvasPressEvent( QgsMapMouseEvent * e ) override;
    void canvasMoveEvent( QgsMapMouseEvent * e ) override;
    void canvasReleaseEvent( QgsMapMouseEvent * e ) override;
    void keyReleaseEvent( QKeyEvent* e ) override;
    void activate() override;
    void deactivate() override;

    void setGeometry(const QgsGeometry &geometry, QgsVectorLayer *layer );

  public slots:
    void pickLine();

  private:
    KadasMapToolDrawPolyLine* mDrawTool;
    QgsRubberBand *mPosMarker;
    KadasHeightProfileDialog* mDialog;
    bool mPicking;

  private slots:
    void drawCleared();
    void drawFinished();
};

#endif // KADASMAPTOOLHEIGHTPROFILE_H
