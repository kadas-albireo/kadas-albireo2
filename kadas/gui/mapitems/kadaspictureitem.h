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
    Q_PROPERTY( bool frame READ frameVisible WRITE setFrameVisible )
    Q_PROPERTY( bool posLocked READ positionLocked WRITE setPositionLocked )

  public:
    KadasPictureItem( const QgsCoordinateReferenceSystem &crs );
    void setup( const QString &path, const KadasItemPos &fallbackPos, bool ignoreExiv = false, double offsetX = 0, double offsetY = 50, int width = 0, int height = 0 );

    const QString &filePath() const { return mFilePath; }
    void setFilePath( const QString &filePath );
    bool frameVisible() const { return mFrame; }
    void setFrameVisible( bool frame );
    bool positionLocked() const { return mPosLocked; }
    void setPositionLocked( bool locked );

    QString itemName() const override { return tr( "Picture" ); }

    QImage symbolImage() const override { return mImage; }

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
    void populateContextMenu( QMenu *menu, const EditContext &context, const KadasMapPos &clickPos, const QgsMapSettings &mapSettings ) override;
    void onDoubleClick( const QgsMapSettings &mapSettings ) override;

    AttribValues editAttribsFromPosition( const EditContext &context, const KadasMapPos &pos, const QgsMapSettings &mapSettings ) const override;
    KadasMapPos positionFromEditAttribs( const EditContext &context, const AttribValues &values, const QgsMapSettings &mapSettings ) const override;

    KadasItemPos position() const override { return constState()->pos; }
    void setPosition( const KadasItemPos &pos ) override;


    struct KADAS_GUI_EXPORT State : KadasMapItem::State
    {
      KadasItemPos pos;
      QList<KadasItemPos> footprint;
      KadasItemPos cameraTarget;
      double angle = 0.;
      double offsetX = 0.;
      double offsetY = 0.;
      QSize size;
      void assign( const KadasMapItem::State *other ) override { *this = *static_cast<const State *>( other ); }
      State *clone() const override SIP_FACTORY { return new State( *this ); }
      QJsonObject serialize() const override;
      bool deserialize( const QJsonObject &json ) override;
    };
    const State *constState() const { return static_cast<State *>( mState ); }
    void setState( const KadasMapItem::State *state ) override;

  protected:
    KadasMapItem *_clone() const override { return new KadasPictureItem( crs() ); } SIP_FACTORY
    State *createEmptyState() const override { return new State(); } SIP_FACTORY

  private:
    enum AttribIds {AttrX, AttrY};
    QString mFilePath;
    QImage mImage;
    bool mFrame = true;
    bool mPosLocked = false;

    static constexpr int sFramePadding = 4;
    static constexpr int sArrowWidth = 6;

    State *state() { return static_cast<State *>( mState ); }

    QList<KadasMapPos> cornerPoints( const QgsMapSettings &settings ) const;
    static bool readGeoPos( const QString &filePath, const QgsCoordinateReferenceSystem &destCrs, KadasItemPos &cameraPos, QList<KadasItemPos> &footprint, KadasItemPos &cameraTarget );
    static double parseExifRational( const QString &rational );
    static QgsPoint findTerrainIntersection( const QgsPoint &cameraPos, QgsPoint nearPos, QgsPoint farPos, const QgsCoordinateReferenceSystem &crs );
};

#endif // KADASPICTUREITEM_H
