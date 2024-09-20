/***************************************************************************
    kadasmapswipecanvasitem.h
    -----------------
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


#ifndef KADASMAPSWIPECANVASITEM_H
#define KADASMAPSWIPECANVASITEM_H

#include <QSet>

#include <qgsmapcanvasitem.h>

class QgsMapLayer;

class KadasMapSwipeCanvasItem : public QgsMapCanvasItem
{
  public:
    KadasMapSwipeCanvasItem( QgsMapCanvas *mapCanvas );

    void enable();
    void disable( bool clearLayers = false );

    //! Sets the layers which will be removed from rendering
    void setLayers(const QSet<QgsMapLayer *> &layers );

    void setPixelPosition( int x , int y );

    void setVertical( bool vertical );

  public slots:
    void refreshMap();

  protected:
    virtual void paint( QPainter *painter ) override;

  private:
    int mPixelLength = std::numeric_limits<double>::quiet_NaN();
    QSet<QgsMapLayer *> mRemovedLayers;
    bool mIsVertical = true;
    QImage mRenderedMapImage;
};

#endif // KADASMAPSWIPECANVASITEM_H
