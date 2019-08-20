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
  KadasLineItem ( const QgsCoordinateReferenceSystem& crs, bool geodesic = false, QObject* parent = nullptr );

  QList<Node> nodes ( const QgsMapSettings& settings ) const override;

  bool startPart ( const QgsPointXY& firstPoint ) override;
  bool startPart ( const AttribValues& values ) override;
  void setCurrentPoint ( const QgsPointXY& p, const QgsMapSettings* mapSettings = nullptr ) override;
  void setCurrentAttributes ( const AttribValues& values ) override;
  bool continuePart() override;
  void endPart() override;

  AttribDefs drawAttribs() const override;
  AttribValues drawAttribsFromPosition ( const QgsPointXY& pos ) const override;
  QgsPointXY positionFromDrawAttribs ( const AttribValues& values ) const override;

  EditContext getEditContext ( const QgsPointXY& pos, const QgsMapSettings& mapSettings ) const override;
  void edit ( const EditContext& context, const QgsPointXY& newPoint, const QgsMapSettings* mapSettings = nullptr ) override;
  void edit ( const EditContext& context, const AttribValues& values ) override;

  AttribValues editAttribsFromPosition ( const EditContext& context, const QgsPointXY& pos ) const override;
  QgsPointXY positionFromEditAttribs ( const EditContext& context, const AttribValues& values, const QgsMapSettings& mapSettings ) const override;

  QgsWkbTypes::GeometryType geometryType() const override { return QgsWkbTypes::LineGeometry; }

  void addPartFromGeometry ( const QgsAbstractGeometry* geom ) override;
  const QgsMultiLineString* geometry() const;

  enum MeasurementMode {
    MeasureLineAndSegments,
    MeasureAzimuthMapNorth,
    MeasureAzimuthGeoNorth
  };

  void setMeasurementMode ( MeasurementMode measurementMode, QgsUnitTypes::AngleUnit angleUnit = QgsUnitTypes::AngleDegrees );
  MeasurementMode measurementMode() const { return mMeasurementMode; }
  QgsUnitTypes::AngleUnit angleUnit() const { return mAngleUnit; }

  struct State : KadasMapItem::State {
    QList<QList<QgsPointXY>> points;
    void assign ( const KadasMapItem::State* other ) override;
    State* clone() const override { return new State ( *this ); }
  };
  const State* constState() const { return static_cast<State*> ( mState ); }

private:
  enum AttribIds {AttrX, AttrY};

  bool mGeodesic = false;
  MeasurementMode mMeasurementMode = MeasureLineAndSegments;
  QgsUnitTypes::AngleUnit mAngleUnit = QgsUnitTypes::AngleDegrees;

  QgsMultiLineString* geometry();
  State* state() { return static_cast<State*> ( mState ); }
  State* createEmptyState() const override { return new State(); }
  void measureGeometry() override;
  void recomputeDerived() override;
};

#endif // KADASLINEITEM_H
