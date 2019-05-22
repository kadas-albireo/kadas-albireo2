/***************************************************************************
    kadaslineitem.h
    ---------------
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

#ifndef KADASLINEITEM_H
#define KADASLINEITEM_H

#include <kadas/core/mapitems/kadasgeometryitem.h>

class QgsMultiLineString;

class KADAS_CORE_EXPORT KadasLineItem : public KadasGeometryItem
{
public:
  KadasLineItem(const QgsCoordinateReferenceSystem& crs, bool geodesic = false, QObject* parent = nullptr);

  bool startPart(const QgsPointXY& firstPoint) override;
  bool startPart(const QList<double>& attributeValues) override;
  void setCurrentPoint(const QgsPointXY& p, const QgsMapSettings& mapSettings) override;
  void setCurrentAttributes(const QList<double>& values) override;
  bool continuePart() override;
  void endPart() override;

  QList<NumericAttribute> attributes() const override;
  QList<double> attributesFromPosition(const QgsPointXY& pos) const override;
  QgsPointXY positionFromAttributes(const QList<double>& values) const override;

  const QgsMultiLineString* geometry() const;

  enum MeasurementMode {
    MeasureLineAndSegments,
    MeasureAzimuthMapNorth,
    MeasureAzimuthGeoNorth
  };

  void setMeasurementMode(MeasurementMode measurementMode, QgsUnitTypes::AngleUnit angleUnit=QgsUnitTypes::AngleDegrees);

private:
  struct State : KadasMapItem::State {
    QList<QList<QgsPointXY>> points;
    void assign(const KadasMapItem::State* other) override;
    State* clone() const override{ return new State(*this); }
  };
  enum Attributes {AttrX, AttrY, NAttrs};

  bool mGeodesic = false;
  MeasurementMode mMeasurementMode = MeasureLineAndSegments;
  QgsUnitTypes::AngleUnit mAngleUnit = QgsUnitTypes::AngleDegrees;

  QgsMultiLineString* geometry();
  State* state() const{ return static_cast<State*>(mState); }
  State* createEmptyState() const override { return new State(); }
  void measureGeometry() override;
  void recomputeDerived() override;
};

#endif // KADASLINEITEM_H
