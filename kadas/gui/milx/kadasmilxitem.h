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
    // Q_OBJECT
    // Q_PROPERTY( QString mssString READ mssString WRITE setMssString )
    // Q_PROPERTY( QString militaryName READ militaryName WRITE setMilitaryName )
    // Q_PROPERTY( int minNPoints READ minNPoints WRITE setMinNPoints )
    // Q_PROPERTY( bool hasVariablePoints READ hasVariablePoints WRITE setHasVariablePoints )
    // Q_PROPERTY( QString symbolType READ symbolType WRITE setSymbolType )

  public:
    KadasMilxItem();
    void setSymbol( const KadasMilxSymbolDesc &symbolDesc );

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

    QImage symbolImage() const override;
    QPointF symbolAnchor() const override { return mSymbolAnchor; }

    QgsRectangle boundingBox() const override;
    Margin margin() const override;

    QList<KadasMapItem::Node> nodes( const QgsMapSettings &settings ) const override;

    bool intersects( const QgsRectangle &rect, const QgsMapSettings &settings, bool contains = false ) const override;
    bool hitTest( const KadasMapPos &pos, const QgsMapSettings &settings ) const override;
    QPair<KadasMapPos, double> closestPoint( const KadasMapPos &pos, const QgsMapSettings &settings ) const override;

    void render( QgsRenderContext &context, QgsFeedback *feedback ) override;
#ifndef SIP_RUN
    QString asKml( const QgsRenderContext &context, QuaZip *kmzZip = nullptr ) const override;
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

    // Draw interface (all points in item crs)
    bool startPart( const KadasMapPos &firstPoint, const QgsMapSettings &mapSettings ) override;
    bool startPart( const AttribValues &values, const QgsMapSettings &mapSettings ) override;
    void setCurrentPoint( const KadasMapPos &p, const QgsMapSettings &mapSettings ) override;
    void setCurrentAttributes( const AttribValues &values, const QgsMapSettings &mapSettings ) override;
    bool continuePart( const QgsMapSettings &mapSettings ) override;
    void endPart() override;

    AttribDefs drawAttribs() const override;
    AttribValues drawAttribsFromPosition( const KadasMapPos &pos, const QgsMapSettings &mapSettings ) const override;
    KadasMapPos positionFromDrawAttribs( const AttribValues &values, const QgsMapSettings &mapSettings ) const override;

    // Edit interface (all points in item crs)
    EditContext getEditContext( const KadasMapPos &pos, const QgsMapSettings &mapSettings ) const override;
    void edit( const EditContext &context, const KadasMapPos &newPoint, const QgsMapSettings &mapSettings ) override;
    void edit( const EditContext &context, const AttribValues &values, const QgsMapSettings &mapSettings ) override;
    void populateContextMenu( QMenu *menu, const EditContext &context, const KadasMapPos &clickPos, const QgsMapSettings &mapSettings ) override;
    void onDoubleClick( const QgsMapSettings &mapSettings ) override;

    AttribValues editAttribsFromPosition( const EditContext &context, const KadasMapPos &pos, const QgsMapSettings &mapSettings ) const override;
    KadasMapPos positionFromEditAttribs( const EditContext &context, const AttribValues &values, const QgsMapSettings &mapSettings ) const override;

    KadasItemPos position() const override;
    void setPosition( const KadasItemPos &pos ) override;

    QgsAbstractGeometry *toGeometry() const;

    bool isMultiPoint() const;

    void writeMilx( QDomDocument &doc, QDomElement &itemElement ) const;
    static KadasMilxItem *fromMilx( const QDomElement &itemElement, const QgsCoordinateTransform &crst, int symbolSize );
    static KadasMilxItem *fromMssStringAndPoints( const QString &mssString, const QList<KadasItemPos> &points );

    static QRect computeScreenExtent( const QgsRectangle &mapExtent, const QgsMapToPixel &mapToPixel );

    static bool validateMssString( const QString &mssString, QString &adjustedMssString SIP_OUT, QString &messages SIP_OUT );

  protected:
    KadasMapItem *_clone() const override SIP_FACTORY { return new KadasMilxItem(); }
    KadasMilxItem::State *createEmptyState() const override SIP_FACTORY { return new State(); }

  private:
    friend class KadasMilxLayer;

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

    // Symbol cache
    mutable QImage mSymbolGraphic;
    mutable QPointF mSymbolAnchor;

    KadasMilxItem::State *state() { return static_cast<State *>( mState ); }

    QList<QPoint> computeScreenPoints( const QgsMapToPixel &mapToPixel, const QgsCoordinateTransform &mapCrst ) const;
    QList<QPair<int, double>> computeScreenAttributes( const QgsMapToPixel &mapToPixel, const QgsCoordinateTransform &mapCrst ) const;
    KadasMilxClient::NPointSymbol toSymbol( const QgsMapToPixel &mapToPixel, const QgsCoordinateReferenceSystem &mapCrs, bool colored = true ) const;
    double metersToPixels( const QgsPointXY &refPoint, const QgsMapToPixel &mapToPixel, const QgsCoordinateTransform &mapCrst ) const;
    void updateSymbol( const QgsMapSettings &mapSettings, const KadasMilxClient::NPointSymbolGraphic &result );
    void updateSymbolMargin( const KadasMilxClient::NPointSymbolGraphic &result );
    const KadasMilxSymbolSettings &symbolSettings() const;

    static void finalize( KadasMilxItem *item, bool isCorridor );
    static void posPointNodeRenderer( QPainter *painter, const QPointF &screenPoint, int nodeSize );
    static void ctrlPointNodeRenderer( QPainter *painter, const QPointF &screenPoint, int nodeSize );
};

#endif // KADASMILXITEM_H
