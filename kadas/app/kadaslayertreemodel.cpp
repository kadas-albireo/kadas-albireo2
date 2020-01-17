/***************************************************************************
    kadaslayertreemodel.cpp
    -----------------------
    copyright            : (C) 2020 by Sandro Mani
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

#include <kadas/app/kadaslayertreemodel.h>

KadasLayerTreeModel::KadasLayerTreeModel( QgsLayerTree *rootNode, QObject *parent )
  : QgsLayerTreeModel( rootNode, parent )
{

}

QVariant KadasLayerTreeModel::data( const QModelIndex &index, int role ) const
{
  if ( role == Qt::DecorationRole )
  {
    // Hide legend icons
    return QVariant();
  }
  return QgsLayerTreeModel::data( index, role );
}
