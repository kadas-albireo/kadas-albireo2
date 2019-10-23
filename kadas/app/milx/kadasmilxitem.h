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

#include <kadas/gui/mapitems/kadasmapitem.h>
#include <kadas/app/milx/kadasmilxclient.h>

// MilX items always in EPSG:4326
class KadasMilxItem : public KadasMapItem
{
    Q_OBJECT
    Q_PROPERTY( QString mssString READ mssString WRITE setMssString )
    Q_PROPERTY( QString militaryName READ militaryName WRITE setMilitaryName )
    Q_PROPERTY( int minNPoints READ minNPoints WRITE setMinNPoints )
    Q_PROPERTY( bool hasVariablePoints READ hasVariablePoints WRITE setHasVariablePoints )

  public:
    KadasMilxItem( QObject *parent = nullptr );
    void setSymbol( const KadasMilxClient::SymbolDesc &symbolDesc );

    const QString &mssString() const { return mMssString; }
    void setMssString( const QString &mssString );
    const QString &militaryName() const { return mMilitaryName; }
    void setMilitaryName( const QString &militaryName );
    int minNPoints() const { return mMinNPoints; }
    void setMinNPoints( int minNPoints );
    bool hasVariablePoints() const { return mHasVariablePoints; }
    void setHasVariablePoints( bool hasVariablePoints );

    QString itemName() const override { return mMilitaryName; }

    KadasItemRect boundingBox() const override;
    Margin margin() const override;

    QList<KadasMapItem::Node> nodes( const QgsMapSettings &settings ) const override;

    bool intersects( const KadasMapRect &rect, const QgsMapSettings &settings ) const override;

    void render( QgsRenderContext &context ) const override;
    QString asKml( const QgsRenderContext &context, QuaZip *kmzZip = nullptr ) const override;

    // State interface
    struct State : KadasMapItem::State
    {
      QList<KadasItemPos> points;
      QMap<KadasMilxClient::AttributeType, double> attributes;
      QMap<KadasMilxClient::AttributeType, KadasItemPos> attributePoints;
      QList<int> controlPoints;
      QPoint userOffset;
      int pressedPoints = 0;

      void assign( const KadasMapItem::State *other ) override { *this = *static_cast<const State *>( other ); }
      State *clone() const override SIP_FACTORY { return new State( *this ); }
      QJsonObject serialize() const override;
      bool deserialize( const QJsonObject &json ) override;
    };
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

    AttribValues editAttribsFromPosition( const EditContext &context, const KadasMapPos &pos, const QgsMapSettings &mapSettings ) const override;
    KadasMapPos positionFromEditAttribs( const EditContext &context, const AttribValues &values, const QgsMapSettings &mapSettings ) const override;

    KadasItemPos position() const override;
    void setPosition( const KadasItemPos &pos ) override;

    QList<QPoint> computeScreenPoints( const QgsMapToPixel &mapToPixel, const QgsCoordinateTransform &mapCrst ) const;
    QList< QPair<int, double> > computeScreenAttributes( const QgsMapToPixel &mapToPixel, const QgsCoordinateTransform &mapCrst ) const;
    bool isMultiPoint() const;
    KadasMilxClient::NPointSymbol toSymbol( const QgsMapToPixel &mapToPixel, const QgsCoordinateReferenceSystem &mapCrs, bool colored = true ) const;

    void writeMilx( QDomDocument &doc, QDomElement &itemElement ) const;
    static KadasMilxItem *fromMilx( const QDomElement &itemElement, const QgsCoordinateTransform &crst, int symbolSize );

    static QRect computeScreenExtent( const QgsRectangle &mapExtent, const QgsMapToPixel &mapToPixel );

  private:
    enum AttribIds
    {
      AttrX = -2,
      AttrY = -1,
      AttrW = KadasMilxClient::AttributeWidth,
      AttrL = KadasMilxClient::AttributeLength,
      AttrR = KadasMilxClient::AttributeRadius,
      AttrA = KadasMilxClient::AttributeAttitude
    };

    QString mMssString;
    QString mMilitaryName;
    int mMinNPoints = -1;
    bool mHasVariablePoints = false;

    // Symbol cache
    mutable QImage mCachedGraphic;
    mutable QPoint mCachedGraphicOffset;
    mutable QgsRectangle mCachedExtent;

    Margin mMargin;

    State *state() { return static_cast<State *>( mState ); }

    State *createEmptyState() const override { return new State(); } SIP_FACTORY

    KadasMapItem *_clone() const override { return new KadasMilxItem(); } SIP_FACTORY

    double metersToPixels( const QgsPointXY &refPoint, const QgsMapToPixel &mapToPixel, const QgsCoordinateTransform &mapCrst ) const;
    void updateSymbol( const QgsMapSettings &mapSettings, const KadasMilxClient::NPointSymbolGraphic &result );

    static void posPointNodeRenderer( QPainter *painter, const QPointF &screenPoint, int nodeSize );
    static void ctrlPointNodeRenderer( QPainter *painter, const QPointF &screenPoint, int nodeSize );

};

#endif // KADASMILXITEM_H
