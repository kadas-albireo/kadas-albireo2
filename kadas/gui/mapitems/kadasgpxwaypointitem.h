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

  public:
    KadasGpxWaypointItem();

    QString itemName() const override { return tr( "Waypoint" ); }

    const QString &name() const { return mName; }
    void setName( const QString &name );

    Margin margin() const override;
    void render( QgsRenderContext &context ) const override;

  protected:
    KadasMapItem *_clone() const override SIP_FACTORY { return new KadasGpxWaypointItem(); }

    QString mName;
    QFont mLabelFont;
    QSize mLabelSize;
};

#endif // KADASGPXWAYPOINTITEM_H
