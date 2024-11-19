/***************************************************************************
    kadasgpxwaypointitem.h
    ----------------------
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

#ifndef KADASGPXWAYPOINTITEM_H
#define KADASGPXWAYPOINTITEM_H

#include "kadas/gui/mapitems/kadaspointitem.h"

class KADAS_GUI_EXPORT KadasGpxWaypointItem : public KadasPointItem
{
    Q_OBJECT
    Q_PROPERTY( QString name READ name WRITE setName )
    Q_PROPERTY( QFont labelFont READ labelFont WRITE setLabelFont )
    Q_PROPERTY( QColor labelColor READ labelColor WRITE setLabelColor )

  public:
    KadasGpxWaypointItem();

    QString itemName() const override { return tr( "Waypoint" ); }
    QString exportName() const override;

    const QString &name() const { return mName; }
    void setName( const QString &name );

    const QFont &labelFont() const { return mLabelFont; }
    void setLabelFont( const QFont &labelFont );

    const QColor &labelColor() const { return mLabelColor; }
    void setLabelColor( const QColor &labelColor );

    Margin margin() const override;
    void render( QgsRenderContext &context ) const override;
#ifndef SIP_RUN
    QString asKml( const QgsRenderContext &context, QuaZip *kmzZip = nullptr ) const override;
#endif

  protected:
    KadasMapItem *_clone() const override SIP_FACTORY { return new KadasGpxWaypointItem(); }

    QString mName;
    QFont mLabelFont;
    QSize mLabelSize;
    QColor mLabelColor;
};

#endif // KADASGPXWAYPOINTITEM_H
