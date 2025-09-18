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

#include "kadas/gui/mapitems/kadasgeometryitem.h"

class QgsMultiSurface;

class KADAS_GUI_EXPORT KadasCircularSectorItem : public KadasGeometryItem
{
    Q_OBJECT

  public:
    KadasCircularSectorItem();

    QString itemName() const override { return QObject::tr( "Circular Sector" ); }

    QList<KadasMapItem::Node> nodes( const QgsMapSettings &settings ) const override;

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

    void addPartFromGeometry( const QgsAbstractGeometry &geom ) override;
    Qgis::GeometryType geometryType() const override { return Qgis::GeometryType::Polygon; }

    KadasItemPos position() const override;
    void setPosition( const KadasItemPos &pos ) override;

    const QgsMultiSurface *geometry() const;

    class KADAS_GUI_EXPORT State : public KadasMapItem::State
    {
      public:
        enum class SectorStatus
        {
          HaveNothing,
          HaveCenter,
          HaveRadius
        };
        SectorStatus sectorStatus = SectorStatus::HaveNothing;
        QList<KadasItemPos> centers;
        QList<double> radii;
        QList<double> startAngles;
        QList<double> stopAngles;
        void assign( const KadasMapItem::State *other ) override { *this = *static_cast<const State *>( other ); }
        State *clone() const override SIP_FACTORY { return new State( *this ); }
        QJsonObject serialize() const override;
        bool deserialize( const QJsonObject &json ) override;
    };
    const State *constState() const { return static_cast<State *>( mState ); }

  protected:
    KadasMapItem *_clone() const override SIP_FACTORY { return new KadasCircularSectorItem(); }
    State *createEmptyState() const override SIP_FACTORY { return new State(); }
    void recomputeDerived() override;
    void measureGeometry() override;

  private:
    enum AttribIds
    {
      AttrX,
      AttrY,
      AttrR,
      AttrA1,
      AttrA2
    };

    QgsMultiSurface *geometry();
    State *state() { return static_cast<State *>( mState ); }
};

#endif // KADASCIRCULARSECTORITEM_H
