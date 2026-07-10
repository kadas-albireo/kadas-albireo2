/***************************************************************************
    kadaslayertreeview.cpp
    ----------------------
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

#include <QDragMoveEvent>
#include <QMimeData>
#include <QPainter>

#include <qgis/qgslayertree.h>
#include <qgis/qgslayertreemodel.h>
#include <qgis/qgslayertreeregistrybridge.h>
#include <qgis/qgsproject.h>

#include "kadaslayertreeview.h"

KadasLayerTreeView::KadasLayerTreeView( QWidget *parent )
  : QgsLayerTreeView( parent )
{}

bool KadasLayerTreeView::isDatasetDrag( const QMimeData *mimeData )
{
  return !mimeData->hasFormat( QStringLiteral( "application/qgis.layertreemodeldata" ) ) && ( mimeData->hasUrls() || mimeData->hasFormat( QStringLiteral( "application/x-vnd.qgis.qgis.uri" ) ) );
}

void KadasLayerTreeView::dragMoveEvent( QDragMoveEvent *event )
{
  if ( isDatasetDrag( event->mimeData() ) )
  {
    const DropTarget target = computeDropTarget( event->position().toPoint() );
    mDropIndicatorRect = target.indicatorRect;
    mDropIndicatorInto = target.into;
    viewport()->update();
    event->accept();
    return;
  }
  clearDropIndicator();
  QgsLayerTreeView::dragMoveEvent( event );
}

void KadasLayerTreeView::dragLeaveEvent( QDragLeaveEvent *event )
{
  clearDropIndicator();
  QgsLayerTreeView::dragLeaveEvent( event );
}

void KadasLayerTreeView::dropEvent( QDropEvent *event )
{
  if ( isDatasetDrag( event->mimeData() ) )
  {
    const DropTarget target = computeDropTarget( event->position().toPoint() );
    clearDropIndicator();
    if ( target.group )
    {
      QgsProject::instance()->layerTreeRegistryBridge()->setLayerInsertionPoint( QgsLayerTreeRegistryBridge::InsertionPoint( target.group, target.row ) );
    }
    // Track the layers added while the drop is handled, to select them afterwards
    QgsMapLayer *addedLayer = nullptr;
    const QMetaObject::Connection connection
      = connect( QgsProject::instance()->layerTreeRegistryBridge(), &QgsLayerTreeRegistryBridge::addedLayersToLayerTree, this, [&addedLayer]( const QList<QgsMapLayer *> &layers ) {
          if ( !layers.isEmpty() )
          {
            addedLayer = layers.last();
          }
        } );
    // Emits datasetsDropped(), whose (direct) handler adds the layer at the
    // insertion point set above and resets the insertion point afterwards.
    QgsLayerTreeView::dropEvent( event );
    disconnect( connection );
    if ( addedLayer )
    {
      setCurrentLayer( addedLayer );
    }
    return;
  }
  clearDropIndicator();
  QgsLayerTreeView::dropEvent( event );
}

void KadasLayerTreeView::paintEvent( QPaintEvent *event )
{
  QgsLayerTreeView::paintEvent( event );
  if ( mDropIndicatorRect.isNull() )
  {
    return;
  }
  QPainter painter( viewport() );
  painter.setRenderHint( QPainter::Antialiasing );
  QPen pen( palette().color( QPalette::Highlight ), 2 );
  painter.setPen( pen );
  if ( mDropIndicatorInto )
  {
    painter.drawRect( QRectF( mDropIndicatorRect ).adjusted( 1, 1, -1, -1 ) );
  }
  else
  {
    painter.drawLine( mDropIndicatorRect.left(), mDropIndicatorRect.top(), mDropIndicatorRect.right(), mDropIndicatorRect.top() );
  }
}

KadasLayerTreeView::DropTarget KadasLayerTreeView::computeDropTarget( const QPoint &pos ) const
{
  DropTarget target;
  QgsLayerTree *root = layerTreeModel() ? layerTreeModel()->rootGroup() : nullptr;
  if ( !root )
  {
    return target;
  }

  const QModelIndex index = indexAt( pos );
  if ( !index.isValid() )
  {
    // Below the last item: append to the root group
    target.group = root;
    target.row = root->children().count();
    target.indicatorRect = indicatorRectForInsertion( root, target.row );
    return target;
  }

  QgsLayerTreeNode *node = index2node( index );
  if ( !node )
  {
    // Legend or other embedded node: treat as "on" its parent layer row
    QModelIndex layerIndex = index;
    while ( layerIndex.isValid() && !index2node( layerIndex ) )
    {
      layerIndex = layerIndex.parent();
    }
    if ( !layerIndex.isValid() || !( node = index2node( layerIndex ) ) )
    {
      target.group = root;
      target.row = root->children().count();
      target.indicatorRect = indicatorRectForInsertion( root, target.row );
      return target;
    }
    // Insert below the layer owning the legend node
    QgsLayerTreeGroup *parent = QgsLayerTree::toGroup( node->parent() ? static_cast<QgsLayerTreeNode *>( node->parent() ) : root );
    target.group = parent;
    target.row = parent->children().indexOf( node ) + 1;
    target.indicatorRect = indicatorRectForInsertion( parent, target.row );
    return target;
  }

  QgsLayerTreeGroup *parent = node->parent() ? QgsLayerTree::toGroup( node->parent() ) : root;
  const int nodeRow = parent->children().indexOf( node );
  const QRect rect = visualRect( index );

  if ( QgsLayerTree::isGroup( node ) )
  {
    // Top quarter: above; bottom quarter: below; middle: into the group
    if ( pos.y() < rect.top() + rect.height() / 4 )
    {
      target.group = parent;
      target.row = nodeRow;
      target.indicatorRect = QRect( rect.left(), rect.top(), viewport()->width() - rect.left(), 0 );
    }
    else if ( pos.y() > rect.bottom() - rect.height() / 4 )
    {
      target.group = parent;
      target.row = nodeRow + 1;
      target.indicatorRect = QRect( rect.left(), rect.bottom() + 1, viewport()->width() - rect.left(), 0 );
    }
    else
    {
      target.group = QgsLayerTree::toGroup( node );
      target.row = 0;
      target.into = true;
      target.indicatorRect = QRect( rect.left(), rect.top(), viewport()->width() - rect.left(), rect.height() );
    }
  }
  else
  {
    // Layer node: upper half inserts above, lower half below
    const bool below = pos.y() >= rect.center().y();
    target.group = parent;
    target.row = nodeRow + ( below ? 1 : 0 );
    const int y = below ? rect.bottom() + 1 : rect.top();
    target.indicatorRect = QRect( rect.left(), y, viewport()->width() - rect.left(), 0 );
  }
  return target;
}

QRect KadasLayerTreeView::indicatorRectForInsertion( QgsLayerTreeGroup *group, int row ) const
{
  const QList<QgsLayerTreeNode *> children = group->children();
  if ( row < children.count() )
  {
    const QRect rect = visualRect( node2index( children[row] ) );
    return QRect( rect.left(), rect.top(), viewport()->width() - rect.left(), 0 );
  }
  if ( !children.isEmpty() )
  {
    const QRect rect = visualRect( node2index( children.last() ) );
    return QRect( rect.left(), rect.bottom() + 1, viewport()->width() - rect.left(), 0 );
  }
  return QRect( 0, 0, viewport()->width(), 0 );
}

void KadasLayerTreeView::clearDropIndicator()
{
  if ( !mDropIndicatorRect.isNull() )
  {
    mDropIndicatorRect = QRect();
    mDropIndicatorInto = false;
    viewport()->update();
  }
}
