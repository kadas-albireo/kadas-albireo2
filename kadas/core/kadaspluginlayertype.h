/***************************************************************************
    kadaspluginlayertype.h
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

#ifndef KADASPLUGINLAYERTYPE_H
#define KADASPLUGINLAYERTYPE_H

#include <QObject>

#include <qgis/qgspluginlayerregistry.h>

#include <kadas/core/kadas_core.h>

class QMenu;

class KADAS_CORE_EXPORT KadasPluginLayerType : public QObject, public QgsPluginLayerType
{
  public:
    KadasPluginLayerType( const QString &name )
      : QgsPluginLayerType( name ) {}

    virtual void addLayerTreeMenuActions( QMenu *menu, QgsPluginLayer *layer ) const {};

  private:
};

#endif // KADASPLUGINLAYERTYPE_H
