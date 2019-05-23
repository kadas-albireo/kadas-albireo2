/***************************************************************************
    kadascircleitem.h
    -----------------
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

#ifndef KADASCIRCLEITEM_H
#define KADASCIRCLEITEM_H

#include <kadas/core/mapitems/kadasgeometryitem.h>

class QgsCurvePolygon;
class QgsMultiSurface;

class KADAS_CORE_EXPORT KadasCircleItem : public KadasGeometryItem
{
public:
  KadasCircleItem(const QgsCoordinateReferenceSystem& crs, bool geodesic = false, QObject* parent = nullptr);

  QList<QgsPointXY> nodes() const override;

  bool startPart(const QgsPointXY& firstPoint) override;
  bool startPart(const AttribValues& attributeValues) override;
  void setCurrentPoint(const QgsPointXY& p, const QgsMapSettings& mapSettings) override;
  void setCurrentAttributes(const AttribValues& values) override;
  bool continuePart() override;
  void endPart() override;

  AttribDefs drawAttribs() const override;
  AttribValues drawAttribsFromPosition(const QgsPointXY& pos) const override;
  QgsPointXY positionFromDrawAttribs(const AttribValues& values) const override;

  EditContext getEditContext(const QgsPointXY& pos, const QgsMapSettings& mapSettings) const override;
  void edit(const EditContext& context, const QgsPointXY& newPoint, const QgsMapSettings& mapSettings) override;

  QgsWkbTypes::GeometryType geometryType() const override { return QgsWkbTypes::PolygonGeometry; }

  const QgsMultiSurface* geometry() const;

private:
  struct State : KadasMapItem::State {
    QList<QgsPointXY> centers;
    QList<double> radii;
    void assign(const KadasMapItem::State* other) override { *this = *static_cast<const State*>(other); }
    State* clone() const override{ return new State(*this); }
  };
  enum AttribIds {AttrX, AttrY, AttrR, NAttrs};

  bool mGeodesic = false;

  QgsMultiSurface* geometry();
  State* state() const{ return static_cast<State*>(mState); }
  State* createEmptyState() const override { return new State(); }
  void measureGeometry() override;
  void recomputeDerived() override;
  void computeCircle(const QgsPointXY& center, double radius, QgsMultiSurface *multiGeom);
  void computeGeoCircle(const QgsPointXY& center, double radius, QgsMultiSurface *multiGeom);
};

#endif // KADASCIRCLEITEM_H
