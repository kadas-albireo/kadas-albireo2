/***************************************************************************
    kadasrectangleitembase.h
    ------------------
    copyright            : (C) 2024 Denis Rouzaud
    email                : denis@opengis.ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef KADASRECTANGLEITEMBASE_H
#define KADASRECTANGLEITEMBASE_H

#include "kadas/gui/mapitems/kadasmapitem.h"


class KADAS_GUI_EXPORT KadasRectangleItemBase : public KadasMapItem SIP_ABSTRACT
{
    Q_OBJECT
    Q_PROPERTY( bool frame READ frameVisible WRITE setFrameVisible )
    Q_PROPERTY( bool posLocked READ positionLocked WRITE setPositionLocked )

  public:
    KadasRectangleItemBase( const QgsCoordinateReferenceSystem &crs );
    virtual ~KadasRectangleItemBase();

    bool frameVisible() const { return mFrame; }
    void setFrameVisible( bool frame );
    bool positionLocked() const { return mPosLocked; }
    void setPositionLocked( bool locked );

    KadasItemRect boundingBox() const override;
    Margin margin() const override;
    QList<KadasMapItem::Node> nodes( const QgsMapSettings &settings ) const override;
    bool intersects( const KadasMapRect &rect, const QgsMapSettings &settings, bool contains = false ) const override;
    void render( QgsRenderContext &context ) const override;

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

    KadasItemPos position() const override { return constState()->mPos; }
    void setPosition( const KadasItemPos &pos ) override;


    struct KADAS_GUI_EXPORT State : KadasMapItem::State
    {
      KadasItemPos mPos;
      QList<KadasItemPos> mFootprint;
      KadasItemPos mRectangleCenterPoint;
      double mAngle = 0.;
      double mOffsetX = 0.;
      double mOffsetY = 0.;
      QSize mSize;
      virtual void assign( const KadasMapItem::State *other ) override { *this = *static_cast<const State *>( other ); }
      virtual State *clone() const override SIP_FACTORY { return new State( *this ); }
      virtual QJsonObject serialize() const override;
      virtual bool deserialize( const QJsonObject &json ) override;
    };
    const State *constState() const { return static_cast<State *>( mState ); }
    virtual void setState( const KadasMapItem::State *state ) override;

  protected:
    State *state() { return static_cast<State *>( mState ); }
    State *createEmptyState() const override SIP_FACTORY { return new State(); }

    bool mPosLocked = false;

  private:
    virtual void renderPrivate( QgsRenderContext &context, const QPointF &center, const QRect &rect, double dpiScale ) const = 0;
    virtual void editPrivate( const KadasMapPos &newPoint, const QgsMapSettings &mapSettings ) = 0;
    virtual void populateContextMenuPrivate(QMenu *menu, const EditContext &context, const KadasMapPos &clickPos, const QgsMapSettings &mapSettings) {}

    friend class KadasProjectMigration;

    enum AttribIds {AttrX, AttrY};
    QImage mImage;
    bool mFrame = true;

    static constexpr int sFramePadding = 4;
    static constexpr int sArrowWidth = 6;


    QList<KadasMapPos> cornerPoints( const QgsMapSettings &settings ) const;
};

#endif // KADASRECTANGLEITEMBASE_H
