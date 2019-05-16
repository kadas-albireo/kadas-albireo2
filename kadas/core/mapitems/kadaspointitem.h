/***************************************************************************
    kadaspointitem.h
    ----------------
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

#ifndef KADASPOINTITEM_H
#define KADASPOINTITEM_H

#include <kadas/core/mapitems/kadasgeometryitem.h>

class KADAS_CORE_EXPORT KadasPointItem : public KadasGeometryItem
{
public:
  KadasPointItem(const QgsCoordinateReferenceSystem& crs, IconType icon = ICON_CIRCLE, QObject* parent = nullptr);

  KadasStateStack::State* cloneState() const override{ return new State(*state()); }

  bool startPart(const QgsPointXY& firstPoint, const QgsMapSettings& mapSettings) override;
  bool moveCurrentPoint(const QgsPointXY& p, const QgsMapSettings& mapSettings) override;
  bool setNextPoint(const QgsPointXY& p, const QgsMapSettings& mapSettings) override;
  void endPart() override;

  const QgsMultiPoint* geometry() const;

private:
  struct State : KadasStateStack::State {
    QList<QgsPointXY> points;
  };

  QgsMultiPoint* geometry();
  State* state() const{ return static_cast<State*>(mState); }
  State* createEmptyState() const override { return new State(); }
  void recomputeDerived() override;
};

#endif // KADASPOINTITEM_H
