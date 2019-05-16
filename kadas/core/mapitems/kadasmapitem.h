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

#include <kadas/core/kadasstatestack.h>
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

  /* Render the item */
  virtual void render( QgsRenderContext &context ) const = 0;

  /* The translation offset in pixels */
  void setTranslationOffset( double dx, double dy );
  QPointF translationOffset() const { return mTranslationOffset; }

  // State interface
  void setState(KadasStateStack::State* state);
  virtual KadasStateStack::State* cloneState() const = 0;

  // Draw interface
  void reset();
  virtual bool startPart(const QgsPointXY& firstPoint, const QgsMapSettings& mapSettings) = 0;
  virtual bool moveCurrentPoint(const QgsPointXY& p, const QgsMapSettings& mapSettings) = 0;
  virtual bool setNextPoint(const QgsPointXY& p, const QgsMapSettings& mapSettings) = 0;
  virtual void endPart() = 0;

  struct NumericAttribute {
      QString name;
      double min;
      double max;
      int decimals;
      std::function<double(const QgsPointXY& pos)> compute;
  };
  typedef QList<NumericAttribute> NumericAttributes;
  virtual const NumericAttributes& attributes() const{ return mAttributes; }

signals:
  void aboutToBeDestroyed();
  void changed();

protected:
  KadasStateStack::State* mState = nullptr;
  QgsCoordinateReferenceSystem mCrs;
  QPointF mTranslationOffset;
  NumericAttributes mAttributes;

private:
  virtual KadasStateStack::State* createEmptyState() const = 0;
  virtual void recomputeDerived() = 0;
};

#endif // KADASMAPITEM_H
