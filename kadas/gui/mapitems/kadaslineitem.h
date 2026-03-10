/***************************************************************************
    kadaslineitem.h
    ---------------
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

#ifndef KADASLINEITEM_H
#define KADASLINEITEM_H

#include "qgis/qgsannotationlineitem.h"

#include "kadas/gui/mapitems/kadasmapitem.h"
#include "kadas/gui/mapitems/kadasgeometryitem.h"

class QgsMultiLineString;

class KADAS_GUI_EXPORT KadasLineItem : public KadasGeometryItem
{
    Q_OBJECT
    Q_PROPERTY( bool geodesic READ geodesic WRITE setGeodesic )

    Q_PROPERTY( QColor mFillColor READ color WRITE setColor )
    Q_PROPERTY( QColor mColor READ color WRITE setColor )
    Q_PROPERTY( double mStrokeWidth READ strokeWidth WRITE setStrokeWidth )
    Q_PROPERTY( Qt::PenStyle mStrokeStyle READ strokeStyle WRITE setStrokeStyle )

  public:
    KadasLineItem( const QgsCoordinateReferenceSystem &crs, bool geodesic = false );

    //virtual QgsAnnotationLineItem *annotationItem() const override { return mQgsItem; }
    QgsAnnotationLineItem *annotationItem( const QgsCoordinateReferenceSystem &crs = QgsCoordinateReferenceSystem() ) const override SIP_FACTORY { return mQgsItem; };

    bool geodesic() const { return mGeodesic; }
    void setGeodesic( bool geodesic );

    QString itemName() const override { return tr( "Line" ); }

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
    void populateContextMenu( QMenu *menu, const EditContext &context, const KadasMapPos &clickPos, const QgsMapSettings &mapSettings ) override;

    AttribValues editAttribsFromPosition( const EditContext &context, const KadasMapPos &pos, const QgsMapSettings &mapSettings ) const override;
    KadasMapPos positionFromEditAttribs( const EditContext &context, const AttribValues &values, const QgsMapSettings &mapSettings ) const override;


    Qgis::GeometryType geometryType() const override { return Qgis::GeometryType::Line; }

    void addPartFromGeometry( const QgsAbstractGeometry &geom ) override;
    const QgsMultiLineString *geometry() const;

    enum class MeasurementMode SIP_MONKEYPATCH_SCOPEENUM
    {
      MeasureLineAndSegments,
      MeasureAzimuthMapNorth,
      MeasureAzimuthGeoNorth,
      MeasureLineAndSegmentsAndAzimuthMapNorth,
      MeasureLineAndSegmentsAndAzimuthGeoNorth
    };

    void setMeasurementMode( MeasurementMode measurementMode, Qgis::AngleUnit angleUnit = Qgis::AngleUnit::Degrees );
    MeasurementMode measurementMode() const { return mMeasurementMode; }
    Qgis::AngleUnit angleUnit() const { return mAngleUnit; }

    struct KADAS_GUI_EXPORT State : KadasMapItem::State
    {
        QList<QList<KadasItemPos>> points;
        void assign( const KadasMapItem::State *other ) override { *this = *static_cast<const State *>( other ); }
        State *clone() const override SIP_FACTORY { return new State( *this ); }
        QJsonObject serialize() const override;
        bool deserialize( const QJsonObject &json ) override;
    };
    const State *constState() const { return static_cast<State *>( mState ); }

    void translate( double dx, double dy ) override;
    QString asKml( const QgsRenderContext &context, QuaZip *kmzZip = nullptr ) const override;


    void render( QgsRenderContext &context ) const override;


  protected:
    KadasMapItem *_clone() const override SIP_FACTORY;
    State *createEmptyState() const override SIP_FACTORY { return new State(); }
    void recomputeDerived() override;
    void measureGeometry() override;

    void setColor( const QColor &color );
    QColor color() const { return mColor; }
    double strokeWidth() const { return mStrokeWidth2; }
    void setStrokeWidth( double width );
    void setStrokeStyle( const Qt::PenStyle &style );
    Qt::PenStyle strokeStyle() const { return mStrokeStyle2; };

    void updateQgsAnnotation();


  private:
    enum AttribIds
    {
      AttrX,
      AttrY
    };

    QgsAnnotationLineItem *mQgsItem = nullptr;

    bool mGeodesic = false;
    MeasurementMode mMeasurementMode = MeasurementMode::MeasureLineAndSegments;
    Qgis::AngleUnit mAngleUnit = Qgis::AngleUnit::Degrees;

    QgsMultiLineString *geometry();
    State *state() { return static_cast<State *>( mState ); }

    double computeSegmentAzimut( const KadasItemPos &p1, const KadasItemPos &p2, bool geoNorth ) const;

    QColor mStrokeColor2 = Qt::red;
    double mStrokeWidth2 = 1;
    QColor mColor = Qt::white;
    //QColor mFillColor2 = Qt::white;
    Qt::PenStyle mStrokeStyle2 = Qt::PenStyle::SolidLine;
};

#endif // KADASLINEITEM_H
