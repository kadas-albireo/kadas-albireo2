/***************************************************************************
    kadascircularsectoritem.h
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

#ifndef KADASCIRCULARSECTORITEM_H
#define KADASCIRCULARSECTORITEM_H

#include <kadas/core/mapitems/kadasgeometryitem.h>

class QgsMultiSurface;

class KADAS_CORE_EXPORT KadasCircularSectorItem : public KadasGeometryItem
{
public:
  KadasCircularSectorItem(const QgsCoordinateReferenceSystem& crs, QObject* parent = nullptr);

  KadasStateStack::State* cloneState() const override{ return new State(*state()); }

  bool startPart(const QgsPointXY& firstPoint, const QgsMapSettings& mapSettings) override;
  bool moveCurrentPoint(const QgsPointXY& p, const QgsMapSettings& mapSettings) override;
  bool setNextPoint(const QgsPointXY& p, const QgsMapSettings& mapSettings) override;
  void endPart() override;

  const QgsMultiSurface* geometry() const;

  void setMeasureGeometry(bool measureGeometry, QgsUnitTypes::AreaUnit areaUnit);

private:
  struct State : KadasStateStack::State {
    enum DrawStatus {Empty, CenterSet, RadiusSet, Finished} drawStatus = Empty;
    QList<QgsPointXY> centers;
    QList<double> radii;
    QList<double> startAngles;
    QList<double> stopAngles;
  };

  QgsUnitTypes::AreaUnit mAreaUnit;

  QgsMultiSurface* geometry();
  State* state() const{ return static_cast<State*>(mState); }
  State* createEmptyState() const override { return new State(); }
  void measureGeometry() override;
  void recomputeDerived() override;
};

#endif // KADASCIRCULARSECTORITEM_H
