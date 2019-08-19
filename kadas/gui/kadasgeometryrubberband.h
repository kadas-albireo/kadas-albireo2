/***************************************************************************
    kadasgeometryrubberband.h
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

#ifndef KADASGEOMETRYRUBBERBAND_H
#define KADASGEOMETRYRUBBERBAND_H

#include <QBrush>
#include <QPen>

#include <qgis/qgsabstractgeometry.h>
#include <qgis/qgsdistancearea.h>
#include <qgis/qgsmapcanvasitem.h>

#include <kadas/gui/kadas_gui.h>

class QgsPoint;
struct QgsVertexId;

class KADAS_GUI_EXPORT KadasGeometryRubberBand: public QObject, public QgsMapCanvasItem
{
  Q_OBJECT
public:
  enum IconType {
    /**
    * No icon is used
    */
    ICON_NONE,
    /**
     * A cross is used to highlight points (+)
     */
    ICON_CROSS,
    /**
     * A cross is used to highlight points (x)
     */
    ICON_X,
    /**
     * A box is used to highlight points (□)
     */
    ICON_BOX,
    /**
     * A circle is used to highlight points (○)
     */
    ICON_CIRCLE,
    /**
     * A full box is used to highlight points (■)
     */
    ICON_FULL_BOX,
    /**
     * A triangle is used to highlight points (△)
     */
    ICON_TRIANGLE,
    /**
     * A full triangle is used to highlight points (▲)
     */
    ICON_FULL_TRIANGLE
  };

  enum MeasurementMode {
    MEASURE_NONE,
    MEASURE_LINE,
    MEASURE_LINE_AND_SEGMENTS,
    MEASURE_ANGLE,
    MEASURE_AZIMUTH,
    MEASURE_POLYGON,
    MEASURE_RECTANGLE,
    MEASURE_CIRCLE
  };

  enum AzimuthNorth {
    AZIMUTH_NORTH_GEOGRAPHIC,
    AZIMUTH_NORTH_MAP
  };

  KadasGeometryRubberBand ( QgsMapCanvas* mapCanvas, QgsWkbTypes::GeometryType geomType = QgsWkbTypes::LineGeometry );
  ~KadasGeometryRubberBand();

  void updatePosition() override;

  /** Sets geometry (takes ownership). Geometry is expected to be in map coordinates */
  void setGeometry ( QgsAbstractGeometry* geom, const QList<QgsVertexId>& hiddenNodes = QList<QgsVertexId>() );
  const QgsAbstractGeometry* geometry() const { return mGeometry; }

  /** Returns whether the geometry contains the specified point */
  bool contains ( const QgsPoint& p, double tol ) const;

  /** Sets the translation offset (offset in map coordinates for drawing geometry) */
  void setTranslationOffset ( double dx, double dy );
  void translationOffset ( double& dx, double& dy ) { dx = mTranslationOffset[0]; dy = mTranslationOffset[1]; }

  void moveVertex ( const QgsVertexId& id, const QgsPoint& newPos );

  void setFillColor ( const QColor& c );
  QColor fillColor() const;
  void setOutlineColor ( const QColor& c );
  QColor outlineColor() const;
  void setOutlineWidth ( int width );
  int outlineWidth() const;
  void setLineStyle ( Qt::PenStyle penStyle );
  Qt::PenStyle lineStyle() const;
  void setBrushStyle ( Qt::BrushStyle brushStyle );
  Qt::BrushStyle brushStyle() const;
  void setIconType ( IconType iconType ) { mIconType = iconType; }
  IconType iconType() const { return mIconType; }
  void setIconSize ( int iconSize );
  int iconSize() const { return mIconSize; }
  void setIconFillColor ( const QColor& c );
  const QColor& iconFillColor() const { return mIconBrush.color(); }
  void setIconOutlineColor ( const QColor& c );
  QColor iconOutlineColor() const { return mIconPen.color(); }
  void setIconOutlineWidth ( int width );
  int iconOutlineWidth() const { return mIconPen.width(); }
  void setIconLineStyle ( Qt::PenStyle penStyle );
  Qt::PenStyle iconLineStyle() const { return mIconPen.style(); }
  void setIconBrushStyle ( Qt::BrushStyle brushStyle );
  Qt::BrushStyle iconBrushStyle() const { return mIconBrush.style(); }
  void setMeasurementMode ( MeasurementMode measurementMode, QgsUnitTypes::DistanceUnit distanceUnit, QgsUnitTypes::AreaUnit areaUnit, QgsUnitTypes::AngleUnit angleUnit = QgsUnitTypes::AngleDegrees, AzimuthNorth azimuthNorth = AZIMUTH_NORTH_GEOGRAPHIC );

  // Custom measurement handler for cases where default measurement method does not work
  class Measurer
  {
  public:
    virtual ~Measurer() {}
    struct Measurement {
      enum Type {Length, Area, Angle} type;
      QString label;
      double value;
    };
    virtual QList<Measurement> measure ( MeasurementMode measurementMode, int part, const QgsAbstractGeometry* geometry, QList<double>& partMeasurements ) = 0;
  };
  // Ownership transferred
  void setMeasurer ( Measurer* measurer ) { mMeasurer = measurer; }

  QString getTotalMeasurement() const;

protected:
  virtual void paint ( QPainter* painter ) override;

private:
  QgsAbstractGeometry* mGeometry;
  QList<QgsVertexId> mHiddenNodes;
  double mTranslationOffset[2];
  QPen mPen;
  QBrush mBrush;
  int mIconSize;
  IconType mIconType;
  QPen mIconPen;
  QBrush mIconBrush;
  QgsWkbTypes::GeometryType mGeometryType;
  QgsDistanceArea mDa;
  MeasurementMode mMeasurementMode;
  QgsUnitTypes::DistanceUnit mDistanceUnit;
  QgsUnitTypes::AreaUnit mAreaUnit;
  QgsUnitTypes::AngleUnit mAngleUnit;
  AzimuthNorth mAzimuthNorth;
  QList<double> mPartMeasurements;
  QList<QGraphicsTextItem*> mMeasurementLabels;
  Measurer* mMeasurer;

  void drawVertex ( QPainter* p, double x, double y );
  QgsRectangle rubberBandRectangle() const;
  void measureGeometry ( QgsAbstractGeometry* geometry, int part );
  QString formatMeasurement ( double value, bool isArea ) const;
  QString formatAngle ( double value ) const;
  void addMeasurements ( const QStringList& measurements, const QgsPoint& mapPos );

private slots:
  void configureDistanceArea();
  void redrawMeasurements();
};

#endif // KADASGEOMETRYRUBBERBAND_H
