/***************************************************************************
    kdasmilxitem.h
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

#ifndef KADASMILXITEM_H
#define KADASMILXITEM_H

#include "kadas/gui/mapitems/kadasmapitem.h"
#include "kadas/gui/milx/kadasmilxclient.h"

// MilX items always in EPSG:4326
class KADAS_GUI_EXPORT KadasMilxItem : public KadasMapItem
{
    Q_OBJECT
    Q_PROPERTY( QString mssString READ mssString WRITE setMssString )
    Q_PROPERTY( QString militaryName READ militaryName WRITE setMilitaryName )
    Q_PROPERTY( int minNPoints READ minNPoints WRITE setMinNPoints )
    Q_PROPERTY( bool hasVariablePoints READ hasVariablePoints WRITE setHasVariablePoints )
    Q_PROPERTY( QString symbolType READ symbolType WRITE setSymbolType )

  public:
    KadasMilxItem();

    const QString &mssString() const { return mMssString; }
    void setMssString( const QString &mssString );
    const QString &militaryName() const { return mMilitaryName; }
    void setMilitaryName( const QString &militaryName );
    int minNPoints() const { return mMinNPoints; }
    void setMinNPoints( int minNPoints );
    bool hasVariablePoints() const { return mHasVariablePoints; }
    void setHasVariablePoints( bool hasVariablePoints );
    QString symbolType() const { return mSymbolType; }
    void setSymbolType( const QString &symbolType );


    QString itemName() const override { return mMilitaryName; }

    QgsRectangle boundingBox() const override;

    QList<KadasMapItem::Node> nodes( const QgsMapSettings & ) const override { return {}; }

    bool intersects( const QgsRectangle &, const QgsMapSettings &, bool = false ) const override { return false; }

    void render( QgsRenderContext &context ) const override;
    QList<QgsAnnotationItem *> annotationItems( const QgsCoordinateReferenceSystem &crs = QgsCoordinateReferenceSystem() ) const override SIP_FACTORY;
#ifndef SIP_RUN
    // Legacy MilX layers are converted to annotation layers at project load
    // by KadasItemLayerMigration, so this code path is never exercised.
    QString asKml( const QgsRenderContext &, QuaZip * = nullptr ) const override { return QString(); }
#endif

    // State interface
    struct KADAS_GUI_EXPORT State : KadasMapItem::State
    {
        QList<KadasItemPos> points;
#ifndef SIP_RUN
        QMap<KadasMilxAttrType, double> attributes;
        QMap<KadasMilxAttrType, KadasItemPos> attributePoints;
#else
        QMap<int, double> attributes;
        QMap<int, KadasItemPos> attributePoints;
#endif
        QList<int> controlPoints;
        QPoint userOffset;
        int pressedPoints = 0;
        Margin margin;

        void assign( const KadasMapItem::State *other ) override { *this = *static_cast<const State *>( other ); }
        State *clone() const override SIP_FACTORY { return new State( *this ); }
        QJsonObject serialize() const override;
        bool deserialize( const QJsonObject &json ) override;
    };
    void setState( const KadasMapItem::State *state ) override;
    const State *constState() const { return static_cast<State *>( mState ); }

    // Draw / edit interface — legacy KadasMilxLayer is converted to
    // QgsAnnotationLayer at project load by KadasItemLayerMigration, so
    // these code paths are unreachable. The new flow uses
    // KadasMilxAnnotationController via the annotation map tools.
    bool startPart( const KadasMapPos &, const QgsMapSettings & ) override { return false; }
    bool startPart( const AttribValues &, const QgsMapSettings & ) override { return false; }
    void setCurrentPoint( const KadasMapPos &, const QgsMapSettings & ) override {}
    void setCurrentAttributes( const AttribValues &, const QgsMapSettings & ) override {}
    bool continuePart( const QgsMapSettings & ) override { return false; }
    void endPart() override {}

    AttribDefs drawAttribs() const override { return {}; }
    AttribValues drawAttribsFromPosition( const KadasMapPos &, const QgsMapSettings & ) const override { return {}; }
    KadasMapPos positionFromDrawAttribs( const AttribValues &, const QgsMapSettings & ) const override { return {}; }

    EditContext getEditContext( const KadasMapPos &, const QgsMapSettings & ) const override { return {}; }
    void edit( const EditContext &, const KadasMapPos &, const QgsMapSettings & ) override {}
    void edit( const EditContext &, const AttribValues &, const QgsMapSettings & ) override {}

    AttribValues editAttribsFromPosition( const EditContext &, const KadasMapPos &, const QgsMapSettings & ) const override { return {}; }
    KadasMapPos positionFromEditAttribs( const EditContext &, const AttribValues &, const QgsMapSettings & ) const override { return {}; }

    KadasItemPos position() const override;
    void setPosition( const KadasItemPos &pos ) override;

    bool isMultiPoint() const;

  protected:
    KadasMapItem *_clone() const override SIP_FACTORY { return new KadasMilxItem(); }
    KadasMilxItem::State *createEmptyState() const override SIP_FACTORY { return new State(); }

  private:
    enum AttribIds
    {
      AttrX = -2,
      AttrY = -1,
      AttrW = MilxAttributeWidth,
      AttrL = MilxAttributeLength,
      AttrR = MilxAttributeRadius,
      AttrA = MilxAttributeAttitude
    };

    QString mMssString;
    QString mMilitaryName;
    // These are only used when first drawing
    int mMinNPoints = -1;
    bool mHasVariablePoints = false;
    QString mSymbolType;

    KadasMilxItem::State *state() { return static_cast<State *>( mState ); }
};

#endif // KADASMILXITEM_H
