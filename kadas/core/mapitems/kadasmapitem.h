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

#include <qgis/qgsabstractgeometry.h>
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
  typedef QMap<int, NumericAttribute> AttribDefs;
  typedef QMap<int, double> AttribValues;

  // Draw interface
  void clear();
  virtual bool startPart(const QgsPointXY& firstPoint) = 0;
  virtual bool startPart(const AttribValues& attributeValues) = 0;
  virtual void setCurrentPoint(const QgsPointXY& p, const QgsMapSettings& mapSettings) = 0;
  virtual void setCurrentAttributes(const AttribValues& values) = 0;
  virtual bool continuePart() = 0;
  virtual void endPart() = 0;

  virtual AttribDefs drawAttribs() const = 0;
  virtual AttribValues drawAttribsFromPosition(const QgsPointXY& pos) const = 0;
  virtual QgsPointXY positionFromDrawAttribs(const AttribValues& values) const = 0;

  // Edit interface
  struct EditContext {
    EditContext(const QgsVertexId& _vidx = QgsVertexId(), const AttribDefs& _attributes = AttribDefs(), Qt::CursorShape _cursor = Qt::CrossCursor)
      : vidx(_vidx)
      , attributes(_attributes)
      , cursor(_cursor)
    {
    }
    QgsVertexId vidx;
    AttribDefs attributes;
    Qt::CursorShape cursor;
    bool isValid() const{ return vidx.isValid(); }
    bool operator==(const EditContext& other) const{ return vidx == other.vidx; }
    bool operator!=(const EditContext& other) const{ return vidx != other.vidx; }
  };
  virtual EditContext getEditContext(const QgsPointXY& pos, const QgsMapSettings& mapSettings) const = 0;
  virtual void edit(const EditContext& context, const QgsPointXY& newPoint, const QgsMapSettings& mapSettings) = 0;

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
