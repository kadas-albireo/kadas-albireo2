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

#include <qgis/qgssettingsentryenumflag.h>
#include <qgis/qgsunittypes.h>

#include "kadas/core/kadassettingstree.h"
#include "kadas/gui/kadas_gui.h"
#include "kadas/gui/kadasbottombar.h"
#include "kadas/gui/kadasmapiteminterface.h"
#include "kadas/gui/mapitemeditors/kadasmapitemeditor.h"
#include "kadas/gui/maptools/kadasmaptoolcreateitem.h"

class QCheckBox;
class QComboBox;
class QLabel;
class QgsGeometry;
class QgsVectorLayer;
class KadasGeometryItem;

class KADAS_GUI_EXPORT KadasMeasureWidget : public KadasMapItemEditor {
  Q_OBJECT
public:
  enum class AzimuthNorth { AzimuthMapNorth, AzimuthGeoNorth };
  Q_ENUM(AzimuthNorth)

  static const inline QgsSettingsEntryEnumFlag<AzimuthNorth>
      *settingsLastAzimuthNorth = new QgsSettingsEntryEnumFlag<AzimuthNorth>(
          QStringLiteral("last-azimuth-north"), KadasSettingsTree::sTreeKadas,
          AzimuthNorth::AzimuthGeoNorth) SIP_SKIP;

  KadasMeasureWidget(KadasMapItem *item);

  void syncItemToWidget() override {}
  void syncWidgetToItem() override;

  void setItem(KadasMapItem *item) override;

signals:
  void clearRequested();
  void pickRequested();

private:
  QLabel *mMeasurementLabel = nullptr;
  QComboBox *mUnitComboBox = nullptr;
  QComboBox *mAngleUnitComboBox = nullptr;
  QComboBox *mNorthComboBox = nullptr;
  QCheckBox *mAzimuthCheckbox = nullptr;

private slots:
  void setDistanceUnit(int index);
  void setAngleUnit(int index);
  void setAzimutEnabled(bool enabled);
  void setAzimuthNorth(int index);
  void updateTotal();
};

class KADAS_GUI_EXPORT KadasMapToolMeasure : public KadasMapToolCreateItem {
  Q_OBJECT
public:
  enum class MeasureMode { MeasureLine, MeasurePolygon, MeasureCircle };
  KadasMapToolMeasure(QgsMapCanvas *canvas, MeasureMode measureMode);

  void activate() override;
  void canvasPressEvent(QgsMapMouseEvent *e) override;
  void canvasMoveEvent(QgsMapMouseEvent *e) override;
  void canvasReleaseEvent(QgsMapMouseEvent *e) override;
  void keyReleaseEvent(QKeyEvent *e) override;

private:
  bool mPickFeature = false;
  MeasureMode mMeasureMode = MeasureMode::MeasureLine;

private slots:
  void requestPick();
};

class KADAS_GUI_EXPORT KadasMapToolMeasureItemInterface
    : public KadasMapItemInterface {
public:
  KadasMapToolMeasureItemInterface(QgsMapCanvas *mapCanvas,
                                   KadasMapToolMeasure::MeasureMode measureMode)
      : KadasMapItemInterface(), mCanvas(mapCanvas), mMeasureMode(measureMode) {
  }
  KadasMapItem *createItem() const override;

private:
  KadasGeometryItem *setupItem(KadasGeometryItem *item) const;
  QgsMapCanvas *mCanvas = nullptr;
  KadasMapToolMeasure::MeasureMode mMeasureMode =
      KadasMapToolMeasure::MeasureMode::MeasureLine;
};

#endif // KADASMAPTOOLMEASURE_H
