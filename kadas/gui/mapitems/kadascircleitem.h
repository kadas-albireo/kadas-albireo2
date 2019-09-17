/***************************************************************************
    kadascircleitem.h
    -----------------
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

#ifndef KADASCIRCLEITEM_H
#define KADASCIRCLEITEM_H

#include <kadas/gui/mapitems/kadasgeometryitem.h>

class QgsCurvePolygon;
class QgsMultiSurface;

class KADAS_GUI_EXPORT KadasCircleItem : public KadasGeometryItem
{
    Q_OBJECT
    Q_PROPERTY( bool geodesic READ geodesic WRITE setGeodesic )

  public:
    KadasCircleItem( const QgsCoordinateReferenceSystem &crs, bool geodesic = false, QObject *parent = nullptr );

    bool geodesic() const { return mGeodesic; }
    void setGeodesic( bool geodesic );

    QString itemName() const override { return tr( "Circle" ); }

    QList<KadasMapItem::Node> nodes( const QgsMapSettings &settings ) const override;

    bool startPart( const QgsPointXY &firstPoint, const QgsMapSettings &mapSettings ) override;
    bool startPart( const AttribValues &values, const QgsMapSettings &mapSettings ) override;
    void setCurrentPoint( const QgsPointXY &p, const QgsMapSettings &mapSettings ) override;
    void setCurrentAttributes( const AttribValues &values, const QgsMapSettings &mapSettings ) override;
    bool continuePart( const QgsMapSettings &mapSettings ) override;
    void endPart() override;

    AttribDefs drawAttribs() const override;
    AttribValues drawAttribsFromPosition( const QgsPointXY &pos ) const override;
    QgsPointXY positionFromDrawAttribs( const AttribValues &values ) const override;

    EditContext getEditContext( const QgsPointXY &pos, const QgsMapSettings &mapSettings ) const override;
    void edit( const EditContext &context, const QgsPointXY &newPoint, const QgsMapSettings &mapSettings ) override;
    void edit( const EditContext &context, const AttribValues &values, const QgsMapSettings &mapSettings ) override;

    AttribValues editAttribsFromPosition( const EditContext &context, const QgsPointXY &pos ) const override;
    QgsPointXY positionFromEditAttribs( const EditContext &context, const AttribValues &values, const QgsMapSettings &mapSettings ) const override;

    void addPartFromGeometry( const QgsAbstractGeometry *geom ) override;
    QgsWkbTypes::GeometryType geometryType() const override { return QgsWkbTypes::PolygonGeometry; }

    const QgsMultiSurface *geometry() const;

    struct State : KadasMapItem::State
    {
      QList<QgsPointXY> centers;
      QList<double> radii;
      void assign( const KadasMapItem::State *other ) override { *this = *static_cast<const State *>( other ); }
      State *clone() const override SIP_FACTORY { return new State( *this ); }
    };
    const State *constState() const { return static_cast<State *>( mState ); }

  protected:
    KadasMapItem *_clone() const override { return new KadasCircleItem( crs() ); } SIP_FACTORY
    State *createEmptyState() const override { return new State(); } SIP_FACTORY
    void recomputeDerived() override;
    void measureGeometry() override;

  private:
    enum AttribIds {AttrX, AttrY, AttrR};

    bool mGeodesic = false;

    QgsMultiSurface *geometry();
    State *state() { return static_cast<State *>( mState ); }
    void computeCircle( const QgsPointXY &center, double radius, QgsMultiSurface *multiGeom );
    void computeGeoCircle( const QgsPointXY &center, double radius, QgsMultiSurface *multiGeom );
};

#endif // KADASCIRCLEITEM_H
