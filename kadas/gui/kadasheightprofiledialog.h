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

class QComboBox;
class QDoubleSpinBox;
class QGroupBox;
class QwtPlot;
class QwtPlotCurve;
class QwtPlotMarker;
class KadasLineItem;
class KadasMapToolHeightProfile;


class KADAS_GUI_EXPORT KadasHeightProfileDialog : public QDialog
{
    Q_OBJECT
  public:
    KadasHeightProfileDialog( KadasMapToolHeightProfile* tool, QWidget* parent = 0, Qt::WindowFlags f = 0 );
    void setPoints(const QList<QgsPointXY> &points, const QgsCoordinateReferenceSystem& crs );
    void setMarkerPos(int segment, const QgsPointXY &p );
    void clear();

  private slots:
    void finish();
    void replot();
    void updateLineOfSight();
    void copyToClipboard();
    void addToCanvas();

  private:
    class ScaleDraw;
    enum HeightMode { HeightRelToGround, HeightRelToSeaLevel };

    KadasMapToolHeightProfile* mTool;
    QwtPlot* mPlot;
    QwtPlotCurve* mPlotCurve;
    QVector<QwtPlotCurve*> mLinesOfSight;
    QVector<KadasLineItem*> mLinesOfSightRB;
    QwtPlotMarker* mPlotMarker;
    QwtPlotMarker* mLineOfSightMarker;
    QList<QwtPlotMarker*> mNodeMarkers;
    QList<QgsPointXY> mPoints;
    QVector<double> mSegmentLengths;
    double mTotLength;
    QgsCoordinateReferenceSystem mPointsCrs;
    int mNSamples;
    QGroupBox* mLineOfSightGroupBoxgroupBox;
    QDoubleSpinBox* mObserverHeightSpinBox;
    QDoubleSpinBox* mTargetHeightSpinBox;
    QComboBox* mHeightModeCombo;

    void keyPressEvent( QKeyEvent *ev ) override;
};

#endif // KADASHEIGHTPROFILEDIALOG_H
