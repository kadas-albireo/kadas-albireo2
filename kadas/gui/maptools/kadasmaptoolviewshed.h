/***************************************************************************
    kadasmaptoolviewshed.h
    ----------------------
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

#ifndef KADASMAPTOOLVIEWSHED_H
#define KADASMAPTOOLVIEWSHED_H

#include <QDialog>

#include <kadas/gui/kadas_gui.h>
#include <kadas/gui/maptools/kadasmaptooldrawshape.h>

class QComboBox;
class QDoubleSpinBox;
class QSlider;

class KADAS_GUI_EXPORT KadasViewshedDialog : public QDialog
{
    Q_OBJECT
  public:
    enum DisplayMode { DisplayVisibleArea, DisplayInvisibleArea };

    KadasViewshedDialog( double radius, QWidget* parent = 0 );
    double getObserverHeight() const;
    double getTargetHeight() const;
    bool getHeightRelativeToGround() const;
    DisplayMode getDisplayMode() const;
    int getAccuracyFactor() const;

  signals:
    void radiusChanged( double radius );

  private:
    enum HeightMode { HeightRelToGround, HeightRelToSeaLevel };
    QDoubleSpinBox* mSpinBoxObserverHeight;
    QDoubleSpinBox* mSpinBoxTargetHeight;
    QComboBox* mComboHeightMode;
    QComboBox* mDisplayModeCombo;
    QSlider* mAccuracySlider;
};

class KADAS_GUI_EXPORT KadasMapToolViewshed : public KadasMapToolDrawCircularSector
{
    Q_OBJECT
  public:
    KadasMapToolViewshed( QgsMapCanvas* mapCanvas );
    void activate();

  private slots:
    void drawFinished();
    void adjustRadius( double newRadius );
};

#endif // KADASMAPTOOLVIEWSHED_H
