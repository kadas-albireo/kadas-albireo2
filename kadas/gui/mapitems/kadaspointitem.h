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

#include "qgis/qgsannotationmarkeritem.h"

#include "kadas/gui/mapitems/kadasmapitem.h"

class KADAS_GUI_EXPORT KadasPointItem : public KadasMapItem
{
    Q_OBJECT

  public:
    KadasPointItem( const QgsCoordinateReferenceSystem &crs, Qgis::MarkerShape icon = Qgis::MarkerShape::Circle );

    virtual QgsAnnotationMarkerItem *annotationItem() const override { return mQgsItem; }

    QString itemName() const override { return tr( "Point" ); }

    void setShape( Qgis::MarkerShape shape );

    bool startPart( const KadasMapPos &firstPoint, const QgsMapSettings &mapSettings ) override;
    bool startPart( const AttribValues &values, const QgsMapSettings &mapSettings ) override;
    void setCurrentPoint( const KadasMapPos &p, const QgsMapSettings &mapSettings ) override;
    void setCurrentAttributes( const AttribValues &values, const QgsMapSettings &mapSettings ) override;
    bool continuePart( const QgsMapSettings &mapSettings ) override;
    void endPart() override;

    virtual QgsRectangle boundingBox() const override;
    virtual QList<KadasMapItem::Node> nodes( const QgsMapSettings &settings ) const override;
    virtual bool intersects( const QgsRectangle &rect, const QgsMapSettings &settings, bool contains ) const override;
    virtual void render( QgsRenderContext &context ) const override;
    virtual QString asKml( const QgsRenderContext &context, QuaZip *kmzZip ) const override;

    AttribDefs drawAttribs() const override;
    AttribValues drawAttribsFromPosition( const KadasMapPos &pos, const QgsMapSettings &mapSettings ) const override;
    KadasMapPos positionFromDrawAttribs( const AttribValues &values, const QgsMapSettings &mapSettings ) const override;

    EditContext getEditContext( const KadasMapPos &pos, const QgsMapSettings &mapSettings ) const override;
    void edit( const EditContext &context, const KadasMapPos &newPoint, const QgsMapSettings &mapSettings ) override;
    void edit( const EditContext &context, const AttribValues &values, const QgsMapSettings &mapSettings ) override;

    AttribValues editAttribsFromPosition( const EditContext &context, const KadasMapPos &pos, const QgsMapSettings &mapSettings ) const override;
    KadasMapPos positionFromEditAttribs( const EditContext &context, const AttribValues &values, const QgsMapSettings &mapSettings ) const override;

    QgsPointXY point() const;
    void setPoint( const QgsPoint &point );

    virtual void translate( double dx, double dy ) override;

    void setIconSize( int size );
    int iconSize() const { return mIconSize; }
    void setColor( const QColor &color );
    QColor color() const { return mFillColor; }
    void setStrokeColor( const QColor &strokeColor );
    QColor strokeColor() const { return mStrokeColor; }
    void setStrokeWidth( double width );
    double strokeWidth() const { return mStrokeWidth; }
    void setStrokeStyle( const Qt::PenStyle &style );
    Qt::PenStyle strokeStyle() const { return mStrokeStyle; }


    //Qgis::GeometryType geometryType() const override { return Qgis::GeometryType::Point; }
    //void addPartFromGeometry( const QgsAbstractGeometry &geom ) override;

    struct KADAS_GUI_EXPORT State : KadasMapItem::State
    {
        QList<KadasItemPos> points;
        void assign( const KadasMapItem::State *other ) override { *this = *static_cast<const State *>( other ); }
        State *clone() const override SIP_FACTORY { return new State( *this ); }
        QJsonObject serialize() const override;
        bool deserialize( const QJsonObject &json ) override;
    };
    const State *constState() const { return static_cast<State *>( mState ); }

    QColor fillColor() const;
    void setFillColor( const QColor &newFillColor );

  protected:
    State *createEmptyState() const override SIP_FACTORY { return new State(); }
    KadasMapItem *_clone() const override;
    // void recomputeDerived() override;

  private:
    void updateSymbol();

    QgsAnnotationMarkerItem *mQgsItem = nullptr;
    Qgis::MarkerShape mShape = Qgis::MarkerShape::Circle;
    int mIconSize = 4;
    QColor mStrokeColor = Qt::red;
    double mStrokeWidth = 1;
    QColor mFillColor = Qt::white;
    Qt::PenStyle mStrokeStyle = Qt::PenStyle::SolidLine;

    enum AttribIds
    {
      AttrX,
      AttrY
    };

    State *state() { return static_cast<State *>( mState ); }
};

#endif // KADASPOINTITEM_H
