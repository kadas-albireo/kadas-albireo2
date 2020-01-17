/***************************************************************************
    kadaslayertreemodel.h
    ---------------------
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

#ifndef KADASLAYERTREEMODEL_H
#define KADASLAYERTREEMODEL_H

#include <qgis/qgslayertreemodel.h>

class KadasLayerTreeModel : public QgsLayerTreeModel
{
    Q_OBJECT

  public:
    explicit KadasLayerTreeModel( QgsLayerTree *rootNode, QObject *parent SIP_TRANSFERTHIS = nullptr );
    QVariant data( const QModelIndex &index, int role = Qt::DisplayRole ) const override;
};

#endif // KADASLAYERTREEMODEL_H
