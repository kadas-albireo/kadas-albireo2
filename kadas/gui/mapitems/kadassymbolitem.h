/***************************************************************************
    kadassymbolitem.h
    -----------------
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

#ifndef KADASSYMBOLITEM_H
#define KADASSYMBOLITEM_H

#include <kadas/gui/mapitems/kadasanchoreditem.h>


class KADAS_GUI_EXPORT KadasSymbolItem : public KadasAnchoredItem
{
    Q_OBJECT
    Q_PROPERTY( QString filePath READ filePath WRITE setFilePath )
    Q_PROPERTY( QString name READ name WRITE setName )
    Q_PROPERTY( QString remarks READ remarks WRITE setRemarks )

  public:
    KadasSymbolItem( const QgsCoordinateReferenceSystem &crs, QObject *parent = nullptr );
    void setup( const QString &path, double anchorX, double anchorY );

    QString itemName() const override { return tr( "Symbol" ); }

    void setFilePath( const QString &path );
    const QString &filePath() const { return mFilePath; }
    void setName( const QString &name ) { mName = name; }
    const QString &name() const { return mName; }
    void setRemarks( const QString &remarks ) { mRemarks = remarks; }
    const QString &remarks() const { return mRemarks; }

    void render( QgsRenderContext &context ) const override;
#ifndef SIP_RUN
    QString asKml( const QgsRenderContext &context, QuaZip *kmzZip = nullptr ) const override;
#endif

  private:
    QString mFilePath;
    QString mName;
    QString mRemarks;
    QImage mImage;
    bool mScalable = false;

    KadasMapItem *_clone() const override { return new KadasSymbolItem( crs() ); } SIP_FACTORY
};


class KADAS_GUI_EXPORT KadasPinItem : public KadasSymbolItem
{
    Q_OBJECT

  public:
    KadasPinItem( const QgsCoordinateReferenceSystem &crs, QObject *parent = nullptr );
};


#endif // KADASSYMBOLITEM_H
