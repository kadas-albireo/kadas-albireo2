/***************************************************************************
    kadasrectangleitem.h
    --------------------
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

#ifndef KADASRECTANGLEITEM_H
#define KADASRECTANGLEITEM_H

#include <kadas/core/mapitems/kadasgeometryitem.h>

class QgsMultiPolygon;

class KADAS_CORE_EXPORT KadasRectangleItem : public KadasGeometryItem
{
public:
  KadasRectangleItem(const QgsCoordinateReferenceSystem& crs, QObject* parent = nullptr);

  bool startPart(const QgsPointXY& firstPoint) override;
  bool startPart(const QList<double>& attributeValues) override;
  void setCurrentPoint(const QgsPointXY& p, const QgsMapSettings& mapSettings) override;
  void setCurrentAttributes(const QList<double>& values) override;
  bool continuePart() override;
  void endPart() override;

  QList<NumericAttribute> attributes() const override;
  QList<double> attributesFromPosition(const QgsPointXY& pos) const override;
  QgsPointXY positionFromAttributes(const QList<double>& values) const override;

  const QgsMultiPolygon* geometry() const;

private:
  struct State : KadasMapItem::State {
    QList<QgsPointXY> p1;
    QList<QgsPointXY> p2;
    void assign(const KadasMapItem::State* other) override { *this = *static_cast<const State*>(other); }
    State* clone() const override{ return new State(*this); }
  };
  enum Attributes {AttrX, AttrY, NAttrs};

  QgsMultiPolygon* geometry();
  State* state() const{ return static_cast<State*>(mState); }
  State* createEmptyState() const override { return new State(); }
  void measureGeometry() override;
  void recomputeDerived() override;
};

#endif // KADASRECTANGLEITEM_H
