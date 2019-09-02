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

#include <kadas/gui/mapitems/kadasgeometryitem.h>

class KADAS_GUI_EXPORT KadasPointItem : public KadasGeometryItem
{
  public:
    KadasPointItem( const QgsCoordinateReferenceSystem &crs, IconType icon = ICON_CIRCLE, QObject *parent = nullptr );

    bool startPart( const QgsPointXY &firstPoint, const QgsMapSettings &mapSettings ) override;
    bool startPart( const AttribValues &values, const QgsMapSettings &mapSettings ) override;
    void setCurrentPoint( const QgsPointXY &p, const QgsMapSettings &mapSettings ) override;
    void setCurrentAttributes( const AttribValues &values, const QgsMapSettings &mapSettings ) override;
    bool continuePart() override;
    void endPart() override;

    AttribDefs drawAttribs() const override;
    AttribValues drawAttribsFromPosition( const QgsPointXY &pos ) const override;
    QgsPointXY positionFromDrawAttribs( const AttribValues &values ) const override;

    EditContext getEditContext( const QgsPointXY &pos, const QgsMapSettings &mapSettings ) const override;
    void edit( const EditContext &context, const QgsPointXY &newPoint, const QgsMapSettings &mapSettings ) override;
    void edit( const EditContext &context, const AttribValues &values, const QgsMapSettings &mapSettings ) override;

    AttribValues editAttribsFromPosition( const EditContext &context, const QgsPointXY &pos ) const override;
    QgsPointXY positionFromEditAttribs( const EditContext &context, const AttribValues &values, const QgsMapSettings &mapSettings ) const override;

    QgsWkbTypes::GeometryType geometryType() const override { return QgsWkbTypes::PointGeometry; }
    void addPartFromGeometry( const QgsAbstractGeometry *geom ) override;

    const QgsMultiPoint *geometry() const;

    struct State : KadasMapItem::State
    {
      QList<QgsPointXY> points;
      void assign( const KadasMapItem::State *other ) override { *this = *static_cast<const State *>( other ); }
      State *clone() const override SIP_FACTORY { return new State( *this ); }
    };
    const State *constState() const { return static_cast<State *>( mState ); }

  private:
    enum AttribIds {AttrX, AttrY};

    QgsMultiPoint *geometry();
    State *state() { return static_cast<State *>( mState ); }
    State *createEmptyState() const override { return new State(); }
    void recomputeDerived() override;
};

#endif // KADASPOINTITEM_H
