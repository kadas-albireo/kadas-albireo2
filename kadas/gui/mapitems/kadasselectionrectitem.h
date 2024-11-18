/***************************************************************************
    kadasselectionrectitem.h
    ------------------------
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

#ifndef KADASSELECTIONRECTITEM_H
#define KADASSELECTIONRECTITEM_H

#include "kadas/gui/mapitems/kadasmapitem.h"


class KADAS_GUI_EXPORT KadasSelectionRectItem : public KadasMapItem
{
    Q_OBJECT

  public:
    KadasSelectionRectItem( const QgsCoordinateReferenceSystem &crs );

    void setSelectedItems( const QList<KadasMapItem *> &items );

    QString itemName() const override { return tr( "Selection" ); }

    KadasItemRect boundingBox() const override;
    Margin margin() const override;
    QList<KadasMapItem::Node> nodes( const QgsMapSettings &settings ) const override { return QList<KadasMapItem::Node>(); }
    bool intersects( const KadasMapRect &rect, const QgsMapSettings &settings, bool contains = false ) const override;
    void render( QgsRenderContext &context ) const override;
#ifndef SIP_RUN
    QString asKml( const QgsRenderContext &context, QuaZip *kmzZip = nullptr ) const override { return QString(); }
#endif

    // Item is not meant to be user-editable, all methods below are stubbed
    struct KADAS_GUI_EXPORT State : KadasMapItem::State
    {
      void assign( const KadasMapItem::State *other ) override { *this = *static_cast<const State *>( other ); }
      State *clone() const override SIP_FACTORY { return new State( *this ); }
      QJsonObject serialize() const override;
      bool deserialize( const QJsonObject &json ) override;
    };

    bool startPart( const KadasMapPos &firstPoint, const QgsMapSettings &mapSettings ) override { return false; }
    bool startPart( const AttribValues &values, const QgsMapSettings &mapSettings ) override { return false; }
    void setCurrentPoint( const KadasMapPos &p, const QgsMapSettings &mapSettings ) override { }
    void setCurrentAttributes( const AttribValues &values, const QgsMapSettings &mapSettings ) override { }
    bool continuePart( const QgsMapSettings &mapSettings ) override { return false; }
    void endPart() override {}

    AttribDefs drawAttribs() const override { return AttribDefs(); }
    AttribValues drawAttribsFromPosition( const KadasMapPos &pos, const QgsMapSettings &mapSettings ) const override { return AttribValues(); }
    KadasMapPos positionFromDrawAttribs( const AttribValues &values, const QgsMapSettings &mapSettings ) const override { return KadasMapPos(); }

    EditContext getEditContext( const KadasMapPos &pos, const QgsMapSettings &mapSettings ) const override { return EditContext(); }
    void edit( const EditContext &context, const KadasMapPos &newPoint, const QgsMapSettings &mapSettings ) override { }
    void edit( const EditContext &context, const AttribValues &values, const QgsMapSettings &mapSettings ) override { }

    AttribValues editAttribsFromPosition( const EditContext &context, const KadasMapPos &pos, const QgsMapSettings &mapSettings ) const override { return AttribValues(); }
    KadasMapPos positionFromEditAttribs( const EditContext &context, const AttribValues &values, const QgsMapSettings &mapSettings ) const override { return KadasMapPos(); }

    KadasItemPos position() const override {return KadasItemPos(); }
    void setPosition( const KadasItemPos &pos ) override {}

  protected:
    KadasMapItem *_clone() const override SIP_FACTORY { return new KadasSelectionRectItem( crs() ); }
    State *createEmptyState() const override SIP_FACTORY { return new State(); }

  private:
    QList<KadasMapItem *> mItems;

    KadasMapRect itemsRect( const QgsCoordinateReferenceSystem &mapCrs, double mup ) const;
};

#endif // KADASPICTUREITEM_H
