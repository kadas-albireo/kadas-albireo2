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

#include <QList>

#include <qgis/qgsdistancearea.h>
#include <qgis/qgsgeometry.h>
#include <qgis/qgssettingsentryenumflag.h>
#include <qgis/qgsunittypes.h>

#include "kadas/core/kadassettingstree.h"
#include "kadas/gui/kadas_gui.h"
#include "kadas/gui/maptools/kadasshapecapturemaptool.h"

class QCheckBox;
class QComboBox;
class QLabel;
class KadasBottomBar;
class KadasMeasureLabelsOverlay;


class KADAS_GUI_EXPORT KadasMapToolMeasure : public KadasShapeCaptureMapTool
{
    Q_OBJECT
  public:
    enum class MeasureMode
    {
      MeasureLine,
      MeasurePolygon,
      MeasureCircle
    };

    enum class AzimuthNorth
    {
      AzimuthMapNorth,
      AzimuthGeoNorth
    };
    Q_ENUM( AzimuthNorth )

    static const inline QgsSettingsEntryEnumFlag<AzimuthNorth> *settingsLastAzimuthNorth
      = new QgsSettingsEntryEnumFlag<AzimuthNorth>( QStringLiteral( "last-azimuth-north" ), KadasSettingsTree::sTreeKadas, AzimuthNorth::AzimuthGeoNorth ) SIP_SKIP;

    KadasMapToolMeasure( QgsMapCanvas *canvas, MeasureMode measureMode );
    ~KadasMapToolMeasure() override;

    void activate() override;
    void deactivate() override;
    void canvasPressEvent( QgsMapMouseEvent *e ) override;
    void canvasReleaseEvent( QgsMapMouseEvent *e ) override;
    void keyReleaseEvent( QKeyEvent *e ) override;

  private slots:
    void requestPick();
    void onShapeCaptured( const QgsGeometry &geometry, const QgsCoordinateReferenceSystem &crs );
    void recomputeReadout();

  private:
    struct Part
    {
        QgsGeometry geometry; // canvas CRS
        QgsPointXY circleCenter;
        double circleRadius = 0.0;
    };

    MeasureMode mMeasureMode;
    bool mPickFeature = false;

    QgsDistanceArea mDa;
    QList<Part> mParts;

    KadasMeasureLabelsOverlay *mLabelsOverlay = nullptr;

    KadasBottomBar *mBottomBar = nullptr;
    QLabel *mTitleLabel = nullptr;
    QLabel *mReadoutLabel = nullptr;
    QComboBox *mUnitComboBox = nullptr;
    QComboBox *mAngleUnitComboBox = nullptr;
    QComboBox *mNorthComboBox = nullptr;
    QCheckBox *mAzimuthCheckbox = nullptr;

    QString formatLength( double meters, Qgis::DistanceUnit unit ) const;
    QString formatArea( double sqMeters, Qgis::AreaUnit unit ) const;
    QString formatAngle( double radians, Qgis::AngleUnit unit ) const;
    double computeSegmentAzimuth( const QgsPointXY &p1, const QgsPointXY &p2, bool geoNorth ) const;
    QString lineReadout( const QgsGeometry &g, double &totalLength ) const;
    QString polygonReadout( const QgsGeometry &g, double &totalArea ) const;
    QString circleReadout( const QgsGeometry &g, double &totalArea ) const;
    void updateCanvasLabels( const QList<Part> &parts );
};

#endif // KADASMAPTOOLMEASURE_H
