/***************************************************************************
    kadasheightprofiledialog.h
    --------------------------
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

#ifndef KADASHEIGHTPROFILEDIALOG_H
#define KADASHEIGHTPROFILEDIALOG_H

#include <QDialog>

#include <qgis/qgscoordinatereferencesystem.h>

#include <kadas/gui/kadas_gui.h>

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QGroupBox;
class QProgressBar;
class QwtPlot;
class QwtPlotCurve;
class QwtPlotMarker;
class KadasLineItem;
class KadasMapToolHeightProfile;


class KADAS_GUI_EXPORT KadasHeightProfileDialog : public QDialog
{
    Q_OBJECT
  public:
    KadasHeightProfileDialog( KadasMapToolHeightProfile *tool, QWidget *parent = 0, Qt::WindowFlags f = 0 );
    void setPoints( const QList<QgsPointXY> &points, const QgsCoordinateReferenceSystem &crs );
    void setMarkerPos( int segment, const QgsPointXY &p );
    void clear();

  protected:
    void keyPressEvent( QKeyEvent *ev ) override;

  private slots:
    void finish();
    void replot();
    void updateLineOfSight();
    void copyToClipboard();
    void addToCanvas();
    void toggleNodeMarkers();

  private:
    class ScaleDraw;
    enum HeightMode { HeightRelToGround, HeightRelToSeaLevel };

    KadasMapToolHeightProfile *mTool = nullptr;
    QwtPlot *mPlot = nullptr;
    QwtPlotCurve *mPlotCurve = nullptr;
    QVector<QwtPlotCurve *> mLinesOfSight;
    QVector<KadasLineItem *> mLinesOfSightRB;
    QwtPlotMarker *mPlotMarker = nullptr;
    QwtPlotMarker *mLineOfSightMarker = nullptr;
    QList<QwtPlotMarker *> mNodeMarkers;
    QList<QgsPointXY> mPoints;
    QVector<double> mSegmentLengths;
    double mTotLength = 0.;
    double mTotLengthMeters = 0.;
    QgsCoordinateReferenceSystem mPointsCrs;
    int mNSamples = 1000;
    QCheckBox *mNodeMarkersCheckbox = nullptr;
    QGroupBox *mLineOfSightGroupBoxgroupBox = nullptr;
    QDoubleSpinBox *mObserverHeightSpinBox = nullptr;
    QDoubleSpinBox *mTargetHeightSpinBox = nullptr;
    QComboBox *mHeightModeCombo = nullptr;
    QProgressBar *mProgressBar = nullptr;
    QPushButton *mCancelButton = nullptr;
};

#endif // KADASHEIGHTPROFILEDIALOG_H
