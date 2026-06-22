/***************************************************************************
    kadasattributetypes.h
    ---------------------
    copyright            : (C) 2026 by Denis Rouzaud
    email                : denis@opengis.ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef KADASATTRIBUTETYPES_H
#define KADASATTRIBUTETYPES_H

#include <QMap>
#include <QPainter>
#include <QPointF>
#include <QString>
#include <Qt>
#include <functional>
#include <limits>

#include <qgis/qgspointxy.h>
#include <qgis/qgsvertexid.h>

#include "kadas/gui/kadas_gui.h"

class QgsMapSettings;


class KADAS_GUI_EXPORT KadasNumericAttribute
{
  public:
    QString name;
    // clang-format off
    enum class Type SIP_MONKEYPATCH_SCOPEENUM_UNNEST( KadasNumericAttribute.Type, Type )
    {
      TypeCoordinate,
      TypeDistance,
      TypeAngle,
      TypeOther
    };
    // clang-format on
    Type type = Type::TypeCoordinate;
    double min = std::numeric_limits<double>::lowest();
    double max = std::numeric_limits<double>::max();
    int decimals = -1;
    int precision( const QgsMapSettings &mapSettings ) const;
    QString suffix( const QgsMapSettings &mapSettings ) const;
};

typedef QMap<int, KadasNumericAttribute> KadasAttribDefs;
typedef QMap<int, double> KadasAttribValues;


//! Editing node descriptor.
struct KADAS_GUI_EXPORT KadasNode
{
    QgsPointXY pos;
#ifndef SIP_RUN
    //! Optional custom handle painter (painter, screen position, size). When unset the edit tool draws its standard handle. std::function allows capturing renderers (e.g. per-node colour or index).
    std::function<void( QPainter *, const QPointF &, int )> render;
#endif
};


//! Editing context for an annotation hit.
struct KADAS_GUI_EXPORT KadasEditContext
{
    //! Geometric precision of a hit; a precise (vertex / handle / edge) hit outranks a body hit in picking.
    enum class HitPrecision
    {
      Body = 0,    //!< Loose hit: click is inside the item's filled body / bounding box / containment area.
      Precise = 1, //!< Tight hit: click is on a vertex, handle, or stroke / outline.
    };

    KadasEditContext() = default;

    //! Valid \a _vidx implies Precise; a default \a _vidx implies Body (upgrade manually for stroke hits).
    KadasEditContext( const QgsVertexId &_vidx, const QgsPointXY &_pos = QgsPointXY(), const KadasAttribDefs &_attributes = KadasAttribDefs(), Qt::CursorShape _cursor = Qt::CrossCursor )
      : mValid( true )
      , vidx( _vidx )
      , pos( _pos )
      , attributes( _attributes )
      , cursor( _cursor )
      , precision( _vidx.isValid() ? HitPrecision::Precise : HitPrecision::Body )
    {}

    bool mValid = false;
    QgsVertexId vidx;
    QgsPointXY pos;
    KadasAttribDefs attributes;
    Qt::CursorShape cursor = Qt::CrossCursor;
    HitPrecision precision = HitPrecision::Body;
    bool isValid() const { return mValid; }

    //! TRUE when both contexts reference the same edit target, i.e. the same vertex identity and validity. \a pos, \a attributes, \a cursor and \a precision are deliberately ignored: they change on every mouse move while the hovered handle stays the same.
    bool isSameTarget( const KadasEditContext &other ) const { return vidx == other.vidx && mValid == other.mValid; }
};

#endif // KADASATTRIBUTETYPES_H
