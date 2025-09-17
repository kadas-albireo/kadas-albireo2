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

#include "kadas/gui/mapitems/kadasgeometryitem.h"

class QgsMultiPolygon;

class KADAS_GUI_EXPORT KadasRectangleItem : public KadasGeometryItem
{
    // Q_OBJECT

  public:
    KadasRectangleItem();

    QString itemName() const override { return QObject::tr( "Rectangle" ); }

    bool startPart( const KadasMapPos &firstPoint, const QgsMapSettings &mapSettings ) override;
    bool startPart( const AttribValues &values, const QgsMapSettings &mapSettings ) override;
    void setCurrentPoint( const KadasMapPos &p, const QgsMapSettings &mapSettings ) override;
    void setCurrentAttributes( const AttribValues &values, const QgsMapSettings &mapSettings ) override;
    bool continuePart( const QgsMapSettings &mapSettings ) override;
    void endPart() override;

    AttribDefs drawAttribs() const override;
    AttribValues drawAttribsFromPosition( const KadasMapPos &pos, const QgsMapSettings &mapSettings ) const override;
    KadasMapPos positionFromDrawAttribs( const AttribValues &values, const QgsMapSettings &mapSettings ) const override;

    EditContext getEditContext( const KadasMapPos &pos, const QgsMapSettings &mapSettings ) const override;
    void edit( const EditContext &context, const KadasMapPos &newPoint, const QgsMapSettings &mapSettings ) override;
    void edit( const EditContext &context, const AttribValues &values, const QgsMapSettings &mapSettings ) override;

    AttribValues editAttribsFromPosition( const EditContext &context, const KadasMapPos &pos, const QgsMapSettings &mapSettings ) const override;
    KadasMapPos positionFromEditAttribs( const EditContext &context, const AttribValues &values, const QgsMapSettings &mapSettings ) const override;

    KadasItemPos position() const override;
    void setPosition( const KadasItemPos &pos ) override;

    void addPartFromGeometry( const QgsAbstractGeometry &geom ) override;
    Qgis::GeometryType geometryType() const override { return Qgis::GeometryType::Polygon; }

    const QgsMultiPolygon *geometry() const;

    struct KADAS_GUI_EXPORT State : KadasMapItem::State
    {
        QList<KadasItemPos> p1;
        QList<KadasItemPos> p2;
        void assign( const KadasMapItem::State *other ) override { *this = *static_cast<const State *>( other ); }
        State *clone() const override SIP_FACTORY { return new State( *this ); }
        QJsonObject serialize() const override;
        bool deserialize( const QJsonObject &json ) override;
    };
    const State *constState() const { return static_cast<State *>( mState ); }

  protected:
    State *createEmptyState() const override SIP_FACTORY { return new State(); }
    void recomputeDerived() override;
    void measureGeometry() override;

  private:
    enum AttribIds
    {
      AttrX,
      AttrY
    };

    KadasMapItem *_clone() const override SIP_FACTORY { return new KadasRectangleItem(); }
    QgsMultiPolygon *geometry();
    State *state() { return static_cast<State *>( mState ); }
};

#endif // KADASRECTANGLEITEM_H
