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
#include <limits>

#include <qgis/qgspointxy.h>
#include <qgis/qgsvertexid.h>

#include "kadas/gui/kadas_gui.h"

class QgsMapSettings;


/**
 * \brief Free-standing replacement for the former \c KadasMapItem::NumericAttribute
 *        nested type. Kept generic so it can be shared between the legacy
 *        \c KadasMapItem chain and the new annotation controller tree.
 */
class KADAS_GUI_EXPORT KadasNumericAttribute
{
  public:
    QString name;
    enum class Type SIP_MONKEYPATCH_SCOPEENUM_UNNEST( KadasNumericAttribute.Type, Type ) { TypeCoordinate, TypeDistance, TypeAngle, TypeOther };
    Type type = Type::TypeCoordinate;
    double min = std::numeric_limits<double>::lowest();
    double max = std::numeric_limits<double>::max();
    int decimals = -1;
    int precision( const QgsMapSettings &mapSettings ) const;
    QString suffix( const QgsMapSettings &mapSettings ) const;
};

typedef QMap<int, KadasNumericAttribute> KadasAttribDefs;
typedef QMap<int, double> KadasAttribValues;


/**
 * \brief Editing node descriptor (formerly \c KadasMapItem::Node). The
 *        position is stored as a generic \c QgsPointXY; legacy callers that
 *        need \c KadasMapPos semantics convert via the implicit constructor.
 */
struct KADAS_GUI_EXPORT KadasNode
{
    QgsPointXY pos;
#ifndef SIP_RUN
    typedef void ( *node_renderer_t )( QPainter *, const QPointF &, int );
    node_renderer_t render = nullptr;
#endif
};


/**
 * \brief Editing context (formerly \c KadasMapItem::EditContext).
 */
struct KADAS_GUI_EXPORT KadasEditContext
{
    KadasEditContext() = default;

    KadasEditContext( const QgsVertexId &_vidx, const QgsPointXY &_pos = QgsPointXY(), const KadasAttribDefs &_attributes = KadasAttribDefs(), Qt::CursorShape _cursor = Qt::CrossCursor )
      : mValid( true )
      , vidx( _vidx )
      , pos( _pos )
      , attributes( _attributes )
      , cursor( _cursor )
    {}

    bool mValid = false;
    QgsVertexId vidx;
    QgsPointXY pos;
    KadasAttribDefs attributes;
    Qt::CursorShape cursor = Qt::CrossCursor;
    bool isValid() const { return mValid; }
    bool operator==( const KadasEditContext &other ) const { return vidx == other.vidx && mValid == other.mValid; }
    bool operator!=( const KadasEditContext &other ) const { return vidx != other.vidx || mValid != other.mValid; }
};

#endif // KADASATTRIBUTETYPES_H
