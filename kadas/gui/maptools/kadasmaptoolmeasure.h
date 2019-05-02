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

#include <qgis/qgsmaptool.h>
#include <qgis/qgsunittypes.h>

#include <kadas/gui/kadas_gui.h>
#include <kadas/gui/kadasbottombar.h>
#include <kadas/gui/kadasgeometryrubberband.h>

class QComboBox;
class QLabel;
class QgsGeometry;
class QgsVectorLayer;
class KadasMapToolDrawShape;
class KadasMeasureWidget;

class KADAS_GUI_EXPORT KadasMapToolMeasure : public QgsMapTool
{
    Q_OBJECT
  public:
    enum MeasureMode { MeasureLine, MeasurePolygon, MeasureCircle, MeasureAngle, MeasureAzimuth };
    KadasMapToolMeasure( QgsMapCanvas* canvas , MeasureMode measureMode );
    ~KadasMapToolMeasure();
    void addGeometry( const QgsGeometry* geometry, const QgsVectorLayer* layer );

    void activate() override;
    void deactivate() override;
    void canvasPressEvent( QgsMapMouseEvent *e ) override;
    void canvasMoveEvent( QgsMapMouseEvent *e ) override;
    void canvasReleaseEvent( QgsMapMouseEvent *e ) override;
    void keyReleaseEvent( QKeyEvent *e ) override;
  private:
    KadasMeasureWidget* mMeasureWidget;
    bool mPickFeature;
    MeasureMode mMeasureMode;
    KadasMapToolDrawShape* mDrawTool;

  private slots:
    void close();
    void setUnits();
    void updateTotal();
    void requestPick();
};

class KADAS_GUI_EXPORT KadasMeasureWidget : public KadasBottomBar
{
    Q_OBJECT
  public:
    KadasMeasureWidget( QgsMapCanvas* canvas, KadasMapToolMeasure::MeasureMode measureMode );
    void updateMeasurement( const QString& measurement );
    QgsUnitTypes::DistanceUnit currentUnit() const;
    QgsUnitTypes::AngleUnit currentAngleUnit() const;
    KadasGeometryRubberBand::AzimuthNorth currentAzimuthNorth() const;

  signals:
    void clearRequested();
    void closeRequested();
    void pickRequested();
    void unitsChanged();

  private:
    QLabel* mMeasurementLabel;
    QComboBox* mUnitComboBox;
    QComboBox* mNorthComboBox;
    KadasMapToolMeasure::MeasureMode mMeasureMode;

  private slots:
    void saveDefaultUnits( int index );
    void saveAzimuthNorth( int index );
};

#endif // KADASMAPTOOLMEASURE_H
