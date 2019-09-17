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

    QString itemName() const override { return tr( "MSS Symbol" ); }

    QgsRectangle boundingBox() const override;
    QRect margin() const override;

    QList<KadasMapItem::Node> nodes( const QgsMapSettings &settings ) const override;

    bool intersects( const QgsRectangle &rect, const QgsMapSettings &settings ) const override;

    void render( QgsRenderContext &context ) const override;

    // State interface
    struct State : KadasMapItem::State
    {
      QList<QgsPointXY> points;
      QList< QPair<int, double> > attributes;
      QList< QPair<int, QgsPointXY> > attributePoints;
      QList<int> controlPoints;
      QPoint userOffset;
      int pressedPoints = 0;

      void assign( const KadasMapItem::State *other ) override { *this = *static_cast<const State *>( other ); }
      State *clone() const override SIP_FACTORY { return new State( *this ); }
    };
    const State *constState() const { return static_cast<State *>( mState ); }

    // Draw interface (all points in item crs)
    bool startPart( const QgsPointXY &firstPoint, const QgsMapSettings &mapSettings ) override;
    bool startPart( const AttribValues &values, const QgsMapSettings &mapSettings ) override;
    void setCurrentPoint( const QgsPointXY &p, const QgsMapSettings &mapSettings ) override;
    void setCurrentAttributes( const AttribValues &values, const QgsMapSettings &mapSettings ) override;
    bool continuePart( const QgsMapSettings &mapSettings ) override;
    void endPart() override;

    AttribDefs drawAttribs() const override;
    AttribValues drawAttribsFromPosition( const QgsPointXY &pos ) const override;
    QgsPointXY positionFromDrawAttribs( const AttribValues &values ) const override;

    // Edit interface (all points in item crs)
    EditContext getEditContext( const QgsPointXY &pos, const QgsMapSettings &mapSettings ) const override;
    void edit( const EditContext &context, const QgsPointXY &newPoint, const QgsMapSettings &mapSettings ) override;
    void edit( const EditContext &context, const AttribValues &values, const QgsMapSettings &mapSettings ) override;

    AttribValues editAttribsFromPosition( const EditContext &context, const QgsPointXY &pos ) const override;
    QgsPointXY positionFromEditAttribs( const EditContext &context, const AttribValues &values, const QgsMapSettings &mapSettings ) const override;

    QList<QPoint> computeScreenPoints( const QgsMapToPixel &mapToPixel, const QgsCoordinateTransform &mapCrst ) const;
    QList< QPair<int, double> > computeScreenAttributes( const QgsMapToPixel &mapToPixel, const QgsCoordinateTransform &mapCrst ) const;
    bool isMultiPoint() const;
    KadasMilxClient::NPointSymbol toSymbol( const QgsMapToPixel &mapToPixel, const QgsCoordinateReferenceSystem &mapCrs, bool colored = true ) const;

    void writeMilx( QDomDocument &doc, QDomElement &itemElement ) const;
    static KadasMilxItem *fromMilx( const QDomElement &itemElement, const QgsCoordinateTransform &crst, int symbolSize );

    static QRect computeScreenExtent( const QgsRectangle &mapExtent, const QgsMapToPixel &mapToPixel );

  private:
    enum AttribIds {AttrX, AttrY};

    QString mMssString;
    QString mMilitaryName;
    int mMinNPoints = -1;
    bool mHasVariablePoints = false;

    QRect mMargin;

    State *state() { return static_cast<State *>( mState ); }

    State *createEmptyState() const override { return new State(); } SIP_FACTORY

    KadasMapItem *_clone() const override { return new KadasMilxItem(); } SIP_FACTORY

    double metersToPixels( const QgsPointXY &refPoint, const QgsMapToPixel &mapToPixel, const QgsCoordinateTransform &mapCrst ) const;
    void updateSymbol( const QgsMapSettings &mapSettings );
    void updateSymbolPoints( const QgsMapSettings &mapSettings, const KadasMilxClient::NPointSymbolGraphic &result );

    static void posPointNodeRenderer( QPainter *painter, const QgsPointXY &screenPoint, int nodeSize );
    static void ctrlPointNodeRenderer( QPainter *painter, const QgsPointXY &screenPoint, int nodeSize );

};

#endif // KADASMILXITEM_H
