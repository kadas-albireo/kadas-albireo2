/***************************************************************************
    kadasvectorlayerproperties.h
    ----------------------------
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

#ifndef KADASVECTORLAYERPROPERTIES_H
#define KADASVECTORLAYERPROPERTIES_H

#include <QDialog>

#include "ui_kadaslayerproperties.h"

class QgsMapLayerConfigWidget;
class QgsMapLayerConfigWidgetFactory;
class QgsRendererPropertiesDialog;
class QgsVectorLayer;


class KadasVectorLayerProperties : public QDialog, Ui::KadasLayerPropertiesBase
{
    Q_OBJECT
  public:
    KadasVectorLayerProperties( QgsVectorLayer *layer, QWidget *parent = nullptr );
    void addPropertiesPageFactory( QgsMapLayerConfigWidgetFactory *factory );

  private slots:
    void apply();

  private:
    QgsVectorLayer *mLayer = nullptr;
    QgsRendererPropertiesDialog *mRendererDialog = nullptr;
    QList<QgsMapLayerConfigWidget *> mLayerPropertiesPages;
};

#endif // KADASVECTORLAYERPROPERTIES_H
