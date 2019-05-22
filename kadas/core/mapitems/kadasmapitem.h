/***************************************************************************
    kadasmapitem.h
    --------------
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

#ifndef KADASMAPITEM_H
#define KADASMAPITEM_H

#include <QObject>

#include <qgis/qgscoordinatereferencesystem.h>
#include <qgis/qgsrectangle.h>

#include <kadas/core/kadasstatehistory.h>
#include <kadas/core/kadas_core.h>

class QgsRenderContext;
struct QgsVertexId;

class KADAS_CORE_EXPORT KadasMapItem : public QObject
{
  Q_OBJECT
public:
  KadasMapItem(const QgsCoordinateReferenceSystem& crs, QObject* parent);
  ~KadasMapItem();

  /* Bounding box in geographic coordinates */
  virtual QgsRectangle boundingBox() const = 0;

  /* The item crs */
  const QgsCoordinateReferenceSystem& crs() const{ return mCrs; }

  /* Margin in pixels */
  virtual int margin() const { return 0; }

  /* Nodes for editing */
  virtual QList<QgsPointXY> nodes() const = 0;

  /* Hit test, rect in item crs */
  virtual bool intersects( const QgsRectangle& rect ) const = 0;

  /* Render the item */
  virtual void render( QgsRenderContext &context ) const = 0;

  /* The translation offset in pixels */
  void setTranslationOffset( double dx, double dy );
  QPointF translationOffset() const { return mTranslationOffset; }

  // State interface
  struct State : KadasStateHistory::State {
    enum DrawStatus { Empty, Drawing, Finished } drawStatus = Empty;
    virtual void assign(const State* other) = 0;
    virtual State* clone() const = 0;
  };
  const State* state() const{ return mState; }
  void setState(const State *state);

  struct NumericAttribute {
      QString name;
      double min;
      double max;
      int decimals;
  };

  // Draw interface
  void clear();
  virtual bool startPart(const QgsPointXY& firstPoint) = 0;
  virtual bool startPart(const QList<double>& attributeValues) = 0;
  virtual void setCurrentPoint(const QgsPointXY& p, const QgsMapSettings& mapSettings) = 0;
  virtual void setCurrentAttributes(const QList<double>& values) = 0;
  virtual bool continuePart() = 0;
  virtual void endPart() = 0;

  virtual QList<NumericAttribute> attributes() const = 0;
  virtual QList<double> attributesFromPosition(const QgsPointXY& pos) const = 0;
  virtual QgsPointXY positionFromAttributes(const QList<double>& values) const = 0;



signals:
  void aboutToBeDestroyed();
  void changed();

protected:
  State* mState = nullptr;
  QgsCoordinateReferenceSystem mCrs;
  QPointF mTranslationOffset;

private:
  virtual State* createEmptyState() const = 0;
  virtual void recomputeDerived() = 0;
};

#endif // KADASMAPITEM_H
