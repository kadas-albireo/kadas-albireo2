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
  struct State : KadasStateStack::State {
    enum DrawStatus { Empty, Drawing, Finished } drawStatus = Empty;
    virtual State* clone() const = 0;
  };
  const State* state() const{ return mState; }
  void setState(State* state);

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
  };
  typedef QList<NumericAttribute> NumericAttributes;
  const NumericAttributes& attributes() const{ return mAttributes; }
  virtual QList<double> recomputeAttributes(const QgsPointXY& pos) const = 0;
  virtual QgsPointXY positionFromAttributes(const QList<double>& values) const = 0;
  virtual bool startPart(const QList<double>& attributeValues) = 0;
  virtual void changeAttributeValues(const QList<double>& values) = 0;
  virtual bool acceptAttributeValues() = 0;


signals:
  void aboutToBeDestroyed();
  void changed();

protected:
  State* mState = nullptr;
  QgsCoordinateReferenceSystem mCrs;
  QPointF mTranslationOffset;
  NumericAttributes mAttributes;

private:
  virtual State* createEmptyState() const = 0;
  virtual void recomputeDerived() = 0;
};

#endif // KADASMAPITEM_H
