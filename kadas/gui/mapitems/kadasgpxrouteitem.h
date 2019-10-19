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

#include <kadas/gui/mapitems/kadaslineitem.h>

class KADAS_GUI_EXPORT KadasGpxRouteItem : public KadasLineItem
{
    Q_OBJECT
    Q_PROPERTY( QString name READ name WRITE setName )
    Q_PROPERTY( QString number READ number WRITE setNumber )

  public:
    KadasGpxRouteItem( QObject *parent = nullptr );

    const QString &name() const { return mName; }
    void setName( const QString &name );

    const QString &number() const { return mNumber; }
    void setNumber( const QString &number );

    Margin margin() const override;
    void render( QgsRenderContext &context ) const override;

  protected:
    QString mName;
    QString mNumber;
    QFont mLabelFont;
    QSize mLabelSize;
};

#endif // KADASGPXROUTEITEM_H
