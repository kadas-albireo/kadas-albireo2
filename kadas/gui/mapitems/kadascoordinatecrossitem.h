/***************************************************************************
    kadascoordinatecrossitem.h
    --------------------------
    copyright            : (C) 2021 by Sandro Mani
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

#ifndef KADASCOORDINATECROSSITEM_H
#define KADASCOORDINATECROSSITEM_H

#include <kadas/gui/mapitems/kadasmapitem.h>

class KADAS_GUI_EXPORT KadasCoordinateCrossItem : public KadasMapItem
{
    Q_OBJECT

  public:
    KadasCoordinateCrossItem( const QgsCoordinateReferenceSystem &crs );

    QString itemName() const override { return tr( "Coordinate cross" ); }

    KadasItemRect boundingBox() const override;
    Margin margin() const override;
    QList<KadasMapItem::Node> nodes( const QgsMapSettings &settings ) const override;
    bool intersects( const KadasMapRect &rect, const QgsMapSettings &settings, bool contains = false ) const override;
    void render( QgsRenderContext &context ) const override;
#ifndef SIP_RUN
    QString asKml( const QgsRenderContext &context, QuaZip *kmzZip = nullptr ) const override;
#endif

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

    struct KADAS_GUI_EXPORT State : KadasMapItem::State
    {
      KadasItemPos pos;
      void assign( const KadasMapItem::State *other ) override { *this = *static_cast<const State *>( other ); }
      State *clone() const override SIP_FACTORY { return new State( *this ); }
      QJsonObject serialize() const override;
      bool deserialize( const QJsonObject &json ) override;
    };
    const State *constState() const { return static_cast<State *>( mState ); }

  protected:
    KadasMapItem *_clone() const override { return new KadasCoordinateCrossItem( crs() ); } SIP_FACTORY
    State *createEmptyState() const override { return new State(); } SIP_FACTORY

  private:
    enum AttribIds {AttrX, AttrY};
    static constexpr int sCrossSize = 60;
    static constexpr int sFontSize = 16;

    State *state() { return static_cast<State *>( mState ); }
    KadasItemPos roundToKilometre( const KadasItemPos &itemPos ) const;
};

#endif // KADASCOORDINATECROSSITEM_H
