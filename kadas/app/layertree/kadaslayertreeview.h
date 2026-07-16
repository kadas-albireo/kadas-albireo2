/***************************************************************************
    kadaslayertreeview.h
    --------------------
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

#ifndef KADASLAYERTREEVIEW_H
#define KADASLAYERTREEVIEW_H

#include <qgis/qgslayertreeview.h>

class QgsLayerTreeGroup;

/**
 * Layer tree view which shows a drop indicator for dataset (e.g. catalog
 * entry) drags and inserts the dropped layer at the indicated position.
 *
 * QgsLayerTreeView accepts such drops and emits datasetsDropped, but bypasses
 * the standard item view drop indicator machinery for them, so without this
 * subclass there is no visual feedback and layers are always inserted at the
 * default insertion point.
 */
class KadasLayerTreeView : public QgsLayerTreeView
{
    Q_OBJECT
  public:
    explicit KadasLayerTreeView( QWidget *parent = nullptr );

  protected:
    void dragMoveEvent( QDragMoveEvent *event ) override;
    void dragLeaveEvent( QDragLeaveEvent *event ) override;
    void dropEvent( QDropEvent *event ) override;
    void paintEvent( QPaintEvent *event ) override;

  private:
    //! Insertion position for a dataset drop, plus the matching indicator shape.
    struct DropTarget
    {
        QgsLayerTreeGroup *group = nullptr;
        int row = 0;
        //! True when dropping into a hovered group rather than between rows.
        bool into = false;
        //! Indicator in viewport coordinates: a zero-height line or, for
        //! into-group drops, the hovered row rectangle.
        QRect indicatorRect;
    };

    //! True for drags carrying dataset URIs (and not internal tree reorders).
    static bool isDatasetDrag( const QMimeData *mimeData );
    DropTarget computeDropTarget( const QPoint &pos ) const;
    //! Line rect at the visual position a node inserted at group/row will take.
    QRect indicatorRectForInsertion( QgsLayerTreeGroup *group, int row ) const;
    void clearDropIndicator();

    //! Active dataset-drag indicator; null when no dataset drag is hovering.
    QRect mDropIndicatorRect;
    bool mDropIndicatorInto = false;
};

#endif // KADASLAYERTREEVIEW_H
