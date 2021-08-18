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
#include <kadas/gui/maptools/kadasmaptoolcreateitem.h>

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QSlider;
class QSpinBox;

class KADAS_GUI_EXPORT KadasViewshedDialog : public QDialog
{
    Q_OBJECT
  public:
    KadasViewshedDialog( double radius, QWidget *parent = 0 );
    double observerHeight() const;
    double targetHeight() const;
    bool observerHeightRelativeToGround() const;
    bool targetHeightRelativeToGround() const;
    double observerMinVertAngle() const;
    double observerMaxVertAngle() const;
    int accuracyFactor() const;

  signals:
    void radiusChanged( double radius );

  private:
    enum HeightMode { HeightRelToGround, HeightRelToSeaLevel };
    QDoubleSpinBox *mSpinBoxObserverHeight = nullptr;
    QDoubleSpinBox *mSpinBoxTargetHeight = nullptr;
    QSpinBox *mSpinBoxObserverMinAngle = nullptr;
    QSpinBox *mSpinBoxObserverMaxAngle = nullptr;
    QComboBox *mComboObserverHeightMode = nullptr;
    QComboBox *mComboTargetHeightMode = nullptr;
    QSlider *mAccuracySlider = nullptr;
    QCheckBox *mVertRangeCheckbox = nullptr;

  private slots:
    void adjustMaxAngle();
    void adjustMinAngle();
};

class KADAS_GUI_EXPORT KadasMapToolViewshed : public KadasMapToolCreateItem
{
    Q_OBJECT
  public:
    KadasMapToolViewshed( QgsMapCanvas *mapCanvas );

  private slots:
    void drawFinished();
    void adjustRadius( double newRadius );

    KadasMapToolCreateItem::ItemFactory itemFactory( const QgsMapCanvas *canvas ) const;
};

#endif // KADASMAPTOOLVIEWSHED_H
