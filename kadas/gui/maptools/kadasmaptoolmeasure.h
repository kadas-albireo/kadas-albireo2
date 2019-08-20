/***************************************************************************
    kadasmaptoolmeasure.h
    ---------------------
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

#ifndef KADASMAPTOOLMEASURE_H
#define KADASMAPTOOLMEASURE_H

#include <qgis/qgsunittypes.h>

#include <kadas/gui/kadas_gui.h>
#include <kadas/gui/kadasbottombar.h>
#include <kadas/gui/mapitemeditors/kadasmapitemeditor.h>
#include <kadas/gui/maptools/kadasmaptoolcreateitem.h>

class QComboBox;
class QLabel;
class QgsGeometry;
class QgsVectorLayer;
class KadasGeometryItem;

class KADAS_GUI_EXPORT KadasMeasureWidget : public KadasMapItemEditor
{
  Q_OBJECT
public:
  enum AzimuthNorth {AzimuthMapNorth, AzimuthGeoNorth};

  KadasMeasureWidget ( KadasMapItem* item, bool measureAzimuth );

  void syncItemToWidget() override {}
  void syncWidgetToItem() override;

signals:
  void clearRequested();
  void pickRequested();

private:
  bool mMeasureAzimuth = false;
  QLabel* mMeasurementLabel = nullptr;
  QComboBox* mUnitComboBox = nullptr;
  QComboBox* mNorthComboBox = nullptr;

private slots:
  void setDistanceUnit ( int index );
  void setAngleUnit ( int index );
  void setAzimuthNorth ( int index );
  void updateTotal();
};

class KADAS_GUI_EXPORT KadasMapToolMeasure : public KadasMapToolCreateItem
{
  Q_OBJECT
public:
  enum MeasureMode { MeasureLine, MeasurePolygon, MeasureCircle, MeasureAzimuth };
  KadasMapToolMeasure ( QgsMapCanvas* canvas, MeasureMode measureMode );

  void activate() override;
  void canvasPressEvent ( QgsMapMouseEvent* e ) override;
  void canvasMoveEvent ( QgsMapMouseEvent* e ) override;
  void canvasReleaseEvent ( QgsMapMouseEvent* e ) override;
  void keyReleaseEvent ( QKeyEvent* e ) override;

private:
  bool mPickFeature = false;
  MeasureMode mMeasureMode = MeasureLine;

  KadasMapToolCreateItem::ItemFactory itemFactory ( QgsMapCanvas* canvas, MeasureMode measureMode ) const;
  KadasGeometryItem* setupItem ( KadasGeometryItem* item, bool measureAzimut ) const;


private slots:
  void requestPick();
};

#endif // KADASMAPTOOLMEASURE_H
