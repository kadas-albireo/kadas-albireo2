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

#include "kadas/gui/mapitems/kadasrectangleitembase.h"


class KADAS_GUI_EXPORT KadasPictureItem : public KadasRectangleItemBase
{
    Q_OBJECT
    Q_PROPERTY( QString filePath READ filePath WRITE setFilePath )

  public:
    KadasPictureItem( const QgsCoordinateReferenceSystem &crs );
    ~KadasPictureItem();
    void setup( const QString &path, const KadasItemPos &fallbackPos, bool ignoreExiv = false, double offsetX = 0, double offsetY = 50, int width = 0, int height = 0 );

    const QString &filePath() const { return mFilePath; }
    void setFilePath( const QString &filePath );

    QString itemName() const override { return tr( "Picture" ); }

    QImage symbolImage() const override { return mImage; }

#ifndef SIP_RUN
    QString asKml( const QgsRenderContext &context, QuaZip *kmzZip = nullptr ) const override;
#endif

    void onDoubleClick( const QgsMapSettings &mapSettings ) override;


    struct KADAS_GUI_EXPORT State : KadasRectangleItemBase::State
    {
      void assign( const KadasMapItem::State *other ) override { *this = *static_cast<const State *>( other ); }
      State *clone() const override SIP_FACTORY { return new State( *this ); }
      QJsonObject serialize() const override;
      bool deserialize( const QJsonObject &json ) override;
    };
    void setState( const KadasMapItem::State *state ) override;

  protected:
    KadasMapItem *_clone() const override SIP_FACTORY { return new KadasPictureItem( crs() ); }
    State *createEmptyState() const override SIP_FACTORY { return new State(); }
    void renderPrivate( QgsRenderContext &context, const QPointF &center, const QRect &rect, double dpiScale ) const override;
    void editPrivate( const KadasMapPos &newPoint, const QgsMapSettings &mapSettings ) override;

  private:
    QImage readImage( double dpiScale = 1 ) const;
    enum AttribIds {AttrX, AttrY};
    QString mFilePath;
    QImage mImage;

    static constexpr int sFramePadding = 4;
    static constexpr int sArrowWidth = 6;

    State *state() { return static_cast<State *>( mState ); }

    static bool readGeoPos( const QString &filePath, const QgsCoordinateReferenceSystem &destCrs, KadasItemPos &cameraPos, QList<KadasItemPos> &footprint, KadasItemPos &cameraTarget );
    static double parseExifRational( const QString &rational );
    static QgsPoint findTerrainIntersection( const QgsPoint &cameraPos, QgsPoint nearPos, QgsPoint farPos, const QgsCoordinateReferenceSystem &crs );
};

#endif // KADASPICTUREITEM_H
