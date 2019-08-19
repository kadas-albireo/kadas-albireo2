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
#include <kadas/gui/maptools/kadasmaptoolcreateitem.h>

class QgsCoordinateReferenceSystem;
class QgsGeometry;
class KadasHeightProfileDialog;
class KadasPointItem;

class KADAS_GUI_EXPORT KadasMapToolHeightProfile : public KadasMapToolCreateItem
{
  Q_OBJECT
public:

  KadasMapToolHeightProfile ( QgsMapCanvas* canvas );
  ~KadasMapToolHeightProfile();

  void canvasPressEvent ( QgsMapMouseEvent* e ) override;
  void canvasMoveEvent ( QgsMapMouseEvent* e ) override;
  void canvasReleaseEvent ( QgsMapMouseEvent* e ) override;
  void keyReleaseEvent ( QKeyEvent* e ) override;
  void activate() override;
  void deactivate() override;

  void setGeometry ( const QgsAbstractGeometry* geom, const QgsCoordinateReferenceSystem& crs );

public slots:
  void pickLine();

private:
  KadasPointItem* mPosMarker = nullptr;
  KadasHeightProfileDialog* mDialog = nullptr;
  bool mPicking = false;

  static ItemFactory lineFactory ( QgsMapCanvas* canvas );

private slots:
  void drawCleared();
  void drawFinished();
};

#endif // KADASMAPTOOLHEIGHTPROFILE_H
