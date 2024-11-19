/***************************************************************************
    kadascoordinatediplayer.h
    -------------------------
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

#ifndef KADASCOORDINATEDISPAYER_H
#define KADASCOORDINATEDISPAYER_H

#include <QTimer>
#include <QWidget>

#include <qgis/qgspoint.h>

#include "kadas/core/kadascoordinateformat.h"
#include "kadas/gui/kadas_gui.h"

class QComboBox;
class QLabel;
class QLineEdit;
class QToolButton;
class QgsCoordinateReferenceSystem;
class QgsMapCanvas;

class KADAS_GUI_EXPORT KadasCoordinateDisplayer : public QWidget
{
    Q_OBJECT
  public:
    KadasCoordinateDisplayer( QToolButton *crsButton, QLineEdit *coordLineEdit, QLineEdit *heightLineEdit, QComboBox *heightCombo, QgsMapCanvas *mapCanvas, QWidget *parent = 0 );
    void getCoordinateDisplayFormat( KadasCoordinateFormat::Format &format, QString &epsg );
    QString getDisplayString( const QgsPointXY &p, const QgsCoordinateReferenceSystem &crs );

  private:
    enum TargetFormat
    {
      LV03,
      LV95,
      DMS,
      DM,
      DD,
      UTM,
      MGRS
    };
    QgsMapCanvas *mMapCanvas;
    QToolButton *mCRSSelectionButton;
    QLineEdit *mCoordinateLineEdit;
    QLineEdit *mHeightLineEdit;
    QComboBox *mHeightSelectionCombo;
    QLabel *mIconLabel;
    QAction *mActionDisplayLV03;
    QAction *mActionDisplayLV95;
    QAction *mActionDisplayDMS;
    QgsPointXY mLastPos;
    QTimer mHeightTimer;

  private slots:
    void displayCoordinates( const QgsPointXY &p );
    void syncProjectCrs();
    void displayFormatChanged( QAction *action );
    void heightUnitChanged( int idx );
    void readProjectSettings();
    void updateHeight();
};

#endif // KADASCOORDINATEDISPAYER_H
