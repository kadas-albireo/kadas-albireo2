/***************************************************************************
    kadasmilxintegration.h
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

#ifndef KADASMILXINTEGRATION_H
#define KADASMILXINTEGRATION_H

#include <QObject>

class QAction;
class QComboBox;
class QSlider;
class QTabWidget;
class QgsMapLayer;
class KadasMilxLayer;
class KadasMilxLibrary;

class KadasMilxIntegration : public QObject
{
    Q_OBJECT
  public:
    struct MilxUi
    {
      QTabWidget *mRibbonWidget;
      QWidget *mMssTab;
      QAction *mActionMilx;
      QAction *mActionSaveMilx;
      QAction *mActionLoadMilx;
      QSlider *mSymbolSizeSlider;
      QSlider *mLineWidthSlider;
      QComboBox *mWorkModeCombo;
    };
    KadasMilxIntegration( const MilxUi &ui, QObject *parent = nullptr );
    ~KadasMilxIntegration();

  private:
    MilxUi mUi;
    KadasMilxLibrary *mMilxLibrary;
    KadasMilxLayer *getLayer();
    KadasMilxLayer *getOrCreateLayer();

    void refreshMilxLayers();
    void showMessageDialog( const QString &title, const QString &body, const QString &messages );

  private slots:
    void createMilx( bool active );
    void saveMilx();
    void loadMilx();
    void setMilXSymbolSize( int value );
    void setMilXLineWidth( int value );
    void setMilXWorkMode( int idx );
};

#endif // KADASMILXINTEGRATION_H
