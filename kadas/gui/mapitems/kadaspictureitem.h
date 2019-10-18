/***************************************************************************
    kadaspictureitem.h
    ------------------
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

#ifndef KADASPICTUREITEM_H
#define KADASPICTUREITEM_H

#include <kadas/gui/mapitems/kadasmapitem.h>


class KADAS_GUI_EXPORT KadasPictureItem : public KadasMapItem
{
    Q_OBJECT
    Q_PROPERTY( QString filePath READ filePath WRITE setFilePath )
    Q_PROPERTY( double offsetX READ offsetX WRITE setOffsetX )
    Q_PROPERTY( double offsetY READ offsetY WRITE setOffsetY )
    Q_PROPERTY( bool frame READ frameVisible WRITE setFrameVisible )
    Q_PROPERTY( bool posLocked READ positionLocked WRITE setPositionLocked )

  public:
    KadasPictureItem( const QgsCoordinateReferenceSystem &crs, QObject *parent = nullptr );
    void setup( const QString &path, const KadasItemPos &fallbackPos, bool ignoreExiv = false, double offsetX = 0, double offsetY = 50 );

    const QString &filePath() const { return mFilePath; }
    void setFilePath( const QString &filePath );
    double offsetX() const { return mOffsetX; }
    void setOffsetX( double offsetX );
    double offsetY() const { return mOffsetY; }
    void setOffsetY( double offsetY );
    bool frameVisible() const { return mFrame; }
    void setFrameVisible( bool frame );
    bool positionLocked() const { return mPosLocked; }
    void setPositionLocked( bool locked );

    QString itemName() const override { return tr( "Picture" ); }

    KadasItemRect boundingBox() const override;
    Margin margin() const override;
    QList<KadasMapItem::Node> nodes( const QgsMapSettings &settings ) const override;
    bool intersects( const KadasMapRect &rect, const QgsMapSettings &settings ) const override;
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
    void populateContextMenu( QMenu *menu, const EditContext &context, const KadasMapPos &clickPos, const QgsMapSettings &mapSettings ) override;

    AttribValues editAttribsFromPosition( const EditContext &context, const KadasMapPos &pos, const QgsMapSettings &mapSettings ) const override;
    KadasMapPos positionFromEditAttribs( const EditContext &context, const AttribValues &values, const QgsMapSettings &mapSettings ) const override;

    KadasItemPos position() const override { return constState()->pos; }
    void setPosition( const KadasItemPos &pos ) override;

    struct State : KadasMapItem::State
    {
      KadasItemPos pos;
      double angle;
      QSize size;
      void assign( const KadasMapItem::State *other ) override { *this = *static_cast<const State *>( other ); }
      State *clone() const override SIP_FACTORY { return new State( *this ); }
    };
    const State *constState() const { return static_cast<State *>( mState ); }

  protected:
    KadasMapItem *_clone() const override { return new KadasPictureItem( crs() ); } SIP_FACTORY
    State *createEmptyState() const override { return new State(); } SIP_FACTORY

  private:
    enum AttribIds {AttrX, AttrY};
    QString mFilePath;
    double mOffsetX = 0;
    double mOffsetY = 0;
    QImage mImage;
    bool mFrame = true;
    bool mPosLocked = false;

    static constexpr int sFramePadding = 4;
    static constexpr int sArrowWidth = 6;

    State *state() { return static_cast<State *>( mState ); }

    QList<KadasMapPos> cornerPoints( const QgsMapSettings &settings ) const;
    static bool readGeoPos( const QString &filePath, QgsPointXY &wgsPos );
};

#endif // KADASPICTUREITEM_H
