/***************************************************************************
    kadasglobedialog.h
    ------------------
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

#ifndef KADASGLOBEDIALOG_H
#define KADASGLOBEDIALOG_H

#include <QDialog>

#include "ui_kadasglobedialog.h"

class KadasGlobeDialog: public QDialog, private Ui::KadasGlobeDialogBase
{
    Q_OBJECT
  public:
    KadasGlobeDialog( QWidget *parent = nullptr, Qt::WindowFlags fl = 0 );

    struct LayerDataSource
    {
      QString uri;
      QString type;
      bool operator==( const LayerDataSource &other ) { return uri == other.uri && type == other.type; }
      bool operator!=( const LayerDataSource &other ) { return uri != other.uri || type != other.type; }
    };
    void readProjectSettings();

    QString getBaseLayerUrl() const;
    bool getSkyEnabled() const;
    QDateTime getSkyDateTime() const;
    bool getSkyAutoAmbience() const;
    double getSkyMinAmbient() const;
    float getScrollSensitivity() const;
    bool getInvertScrollWheel() const;
    QList<LayerDataSource> getImageryDataSources() const;
    QList<LayerDataSource> getElevationDataSources() const;
    double getVerticalScale() const;
    bool getFrustumHighlighting() const;

  signals:
    void settingsApplied();

  private:
    void restoreSavedSettings();
    void writeProjectSettings();
    bool validateRemoteUri( const QString &uri, QString &errMsg ) const;

  private slots:
    void apply();
    void on_buttonBox_accepted();
    void on_buttonBox_rejected();

    void on_comboBoxStereoMode_currentIndexChanged( int index );
    void on_pushButtonStereoResetDefaults_clicked();

    void on_mRemoveImageryButton_clicked();
    void on_mRemoveElevationButton_clicked();

    void addImagery( const QString &type, const QString &uri );
    void addTMSImagery();
    void addCustomTMSImagery();
    void addCustomWMSImagery();
    void addRasterImagery();
    void addCustomRasterImagery();
    void addElevation( const QString &type, const QString &uri );
    void addTMSElevation();
    void addCustomTMSElevation();
    void addCustomRasterElevation();
};

#endif // QGSGLOBEPLUGINDIALOG_H
