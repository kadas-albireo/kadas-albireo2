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
#include <qgis/qgssettingsentryimpl.h>

#include "kadas/core/kadassettingstree.h"
#include "kadas/gui/kadas_gui.h"

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QGroupBox;
class QLabel;
class QPainter;
class QProgressBar;

class QwtPlot;
class QwtPlotCurve;
class QwtPlotMarker;
class QwtPlotPicker;

class KadasLineItem;
class KadasMapToolHeightProfile;


class KADAS_GUI_EXPORT KadasHeightProfileDialog : public QDialog
{
    Q_OBJECT
  public:
    KadasHeightProfileDialog( KadasMapToolHeightProfile *tool, QWidget *parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags() );
    void setPoints( const QList<QgsPointXY> &points, const QgsCoordinateReferenceSystem &crs );
    void setMarkerPos( int segment, const QgsPointXY &p, const QgsCoordinateReferenceSystem &crs );
    void clear();
    bool isBusy() const;

  public slots:
    void accept() override;
    void reject() override;

  protected:
    void keyPressEvent( QKeyEvent *ev ) override;

  private slots:
    void finish();
    void replot();
    void updateLineOfSight();
    void copyToClipboard();
    void saveToFile();
    void addToCanvas();
    void setMarkerPlotPos( const QPoint &pos );
    void toggleNodeMarkers();

    QImage renderPlotImage();

  private:
    class ScaleDraw;
    enum HeightMode
    {
      HeightRelToGround,
      HeightRelToSeaLevel
    };

    enum class Statistics
    {
      HeightDifference,
      TotalAscent,
      TotalDescent,
      MaxHeight,
      MinHeight,
      LinearDistance,
      PathDistance
    };

    QMap<Statistics, double> mStatisticsValues;
    QMap<Statistics, QLabel *> mStatisticsLabels;

    KadasMapToolHeightProfile *mTool = nullptr;
    QwtPlot *mPlot = nullptr;
    QVector<QPointF> mPlotSamples;
    double mNoDataValue = 0.;
    QVector<QwtPlotCurve *> mPlotCurves;
    QVector<QwtPlotCurve *> mLinesOfSight;
    QVector<KadasLineItem *> mLinesOfSightRB;
    QwtPlotMarker *mPlotMarker = nullptr;
    QwtPlotPicker *mPlotPicker = nullptr;
    QwtPlotMarker *mLineOfSightMarker = nullptr;
    QList<QwtPlotMarker *> mNodeMarkers;
    QList<QgsPointXY> mPoints;
    QVector<double> mSegmentLengths;
    double mTotLength = 0;
    double mTotLengthMeters = 0;
    //! Linear distance i.e. direct from start to end point
    double mTotLinearDistanceMeters = 0;
    QgsCoordinateReferenceSystem mPointsCrs;
    int mNSamples = 1000;
    QCheckBox *mNodeMarkersCheckbox = nullptr;
    QCheckBox *mShowStatisticsCheckbox = nullptr;
    QGroupBox *mLineOfSightGroupBoxgroupBox = nullptr;
    QDoubleSpinBox *mObserverHeightSpinBox = nullptr;
    QDoubleSpinBox *mTargetHeightSpinBox = nullptr;
    QComboBox *mHeightModeCombo = nullptr;
    QProgressBar *mProgressBar = nullptr;
    QPushButton *mCancelButton = nullptr;
    QWidget *mStatisticsWidget = nullptr;

    static inline QgsSettingsTreeNode *sTreeProfile = KadasSettingsTree::sTreeKadas->createChildNode( QStringLiteral( "height-profile" ) );
    static const inline QgsSettingsEntryBool *sSettingsShowStatistics = new QgsSettingsEntryBool( QStringLiteral( "show-statistics" ), sTreeProfile, true );
    static const inline QgsSettingsEntryBool *sSettingsNodeMarkers = new QgsSettingsEntryBool( QStringLiteral( "node-markers" ), sTreeProfile, true );
    static const inline QgsSettingsEntryString *sSettingsSaveDir = new QgsSettingsEntryString( QStringLiteral( "save-dir" ), sTreeProfile, QDir::homePath() );
};

#endif // KADASHEIGHTPROFILEDIALOG_H
