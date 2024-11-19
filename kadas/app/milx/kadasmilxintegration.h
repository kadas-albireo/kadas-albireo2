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

#include <qgis/qgscustomdrophandler.h>

#include "kadas/gui/kadasmapiteminterface.h"


class QAction;
class QComboBox;
class QSlider;
class QSpinBox;
class QTabWidget;
class QgsColorButton;
class QgsMapLayer;
class KadasMilxLayer;
class KadasMilxLayerPropertiesPageFactory;
class KadasMilxLibrary;


class KadasMilxInterface : public KadasMapItemInterface
{
  public:
    KadasMilxInterface() = default;
    KadasMapItem *createItem() const override;
};

class KadasMilxDropHandler : public QgsCustomDropHandler
{
    Q_OBJECT

  public:
    bool canHandleMimeData( const QMimeData *data ) override;
    bool handleMimeDataV2( const QMimeData *data ) override;
};

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
        QAction *mActionMilxKmlExport;
        QAction *mActionLoadMilx;
        QSlider *mSymbolSizeSlider;
        QSlider *mLineWidthSlider;
        QComboBox *mWorkModeCombo;
        QSpinBox *mLeaderLineWidthSpin;
        QgsColorButton *mLeaderLineColorButton;
    };
    KadasMilxIntegration( const MilxUi &ui, QObject *parent = nullptr );
    ~KadasMilxIntegration();

    static bool importMilxly( const QString &filename, QString &errorMsg );

  private:
    MilxUi mUi;
    KadasMilxLibrary *mMilxLibrary = nullptr;
    KadasMilxLayerPropertiesPageFactory *mLayerPropertiesFactory = nullptr;
    KadasMilxDropHandler mDropHandler;
    KadasMilxLayer *getLayer();
    KadasMilxLayer *getOrCreateLayer();

    void refreshMilxLayers();
    static void showMessageDialog( const QString &title, const QString &body, const QString &messages );

  private slots:
    void createMilx( bool active );
    void saveMilxly();
    void exportKml();
    void openMilxly();
    void readProjectSettings();
    void setMilXSymbolSize( int value );
    void setMilXLineWidth( int value );
    void setMilXWorkMode( int idx );
    void setMilxLeaderLineWidth( int width );
    void setMilxLeaderLineColor( QColor color );
};

#endif // KADASMILXINTEGRATION_H
