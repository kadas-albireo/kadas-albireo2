/***************************************************************************
    kadasgpxrouteitem.h
    -------------------
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

#ifndef KADASGPXROUTEITEM_H
#define KADASGPXROUTEITEM_H

#include "kadas/gui/mapitems/kadaslineitem.h"

class KADAS_GUI_EXPORT KadasGpxRouteItem : public KadasLineItem
{
    Q_OBJECT
    Q_PROPERTY( QString name READ name WRITE setName )
    Q_PROPERTY( QString number READ number WRITE setNumber )
    Q_PROPERTY( QFont labelFont READ labelFont WRITE setLabelFont )
    Q_PROPERTY( QColor labelColor READ labelColor WRITE setLabelColor )

  public:
    KadasGpxRouteItem( QObject *parent = nullptr );

    QString itemName() const override { return tr( "Route" ); }
    QString exportName() const override;

    const QString &name() const { return mName; }
    void setName( const QString &name );

    const QString &number() const { return mNumber; }
    void setNumber( const QString &number );

    const QFont &labelFont() const { return mLabelFont; }
    void setLabelFont( const QFont &labelFont );

    const QColor &labelColor() const { return mLabelColor; }
    void setLabelColor( const QColor &labelColor );

    Margin margin() const override;
    void render( QgsRenderContext &context ) const override;

  protected:
    KadasMapItem *_clone() const override { return new KadasGpxRouteItem( ); } SIP_FACTORY

    QString mName;
    QString mNumber;
    QFont mLabelFont;
    QSize mLabelSize;
    QColor mLabelColor;
};

#endif // KADASGPXROUTEITEM_H
