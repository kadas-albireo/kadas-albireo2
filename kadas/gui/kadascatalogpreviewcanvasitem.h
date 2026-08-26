/***************************************************************************
    kadascatalogpreviewcanvasitem.h
    -------------------------------
    copyright            : (C) 2026 by OPENGIS.ch
    email                : info@opengis.ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef KADASCATALOGPREVIEWCANVASITEM_H
#define KADASCATALOGPREVIEWCANVASITEM_H

#include <QImage>

#include <qgis/qgsmapcanvasitem.h>

#include "kadas/gui/kadas_gui.h"

/**
 * Canvas overlay showing the rendered image of a catalog entry which is not
 * part of the project.
 *
 * The item only paints what it is given; KadasCatalogPreview builds the layer
 * and runs the render job. While a new image is on its way the previous one is
 * stretched to the current extent, the way the map canvas itself does.
 */
class KADAS_GUI_EXPORT KadasCatalogPreviewCanvasItem : public QgsMapCanvasItem
{
  public:
    explicit KadasCatalogPreviewCanvasItem( QgsMapCanvas *mapCanvas );

    /**
     * Shows \a image, rendered for \a extent.
     */
    void setImage( const QImage &image, const QgsRectangle &extent );

    //! Drops the image, leaving nothing painted.
    void clear();

  protected:
    void paint( QPainter *painter ) override;

  private:
    QImage mImage;
};

#endif // KADASCATALOGPREVIEWCANVASITEM_H
