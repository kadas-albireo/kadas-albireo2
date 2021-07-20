/***************************************************************************
    kadasitemlayerproperties.h
    --------------------------
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

#ifndef KADASITEMLAYERPROPERTIES_H
#define KADASITEMLAYERPROPERTIES_H

#include <qgis/qgsmaplayerconfigwidget.h>

#include <kadas/gui/kadaslayerpropertiesdialog.h>

class QGroupBox;
class QgsScaleRangeWidget;
class KadasItemLayer;


class KadasItemLayerRenderingPropertiesWidget : public QgsMapLayerConfigWidget
{
    Q_OBJECT

  public:
    explicit KadasItemLayerRenderingPropertiesWidget( KadasItemLayer *layer, QgsMapCanvas *canvas, QWidget *parent = nullptr );

  public slots:
    void apply() override;

  private:
    QGroupBox *mGroupBox = nullptr;
    QgsScaleRangeWidget *mScaleRangeWidget = nullptr;
};

class KadasItemLayerProperties : public KadasLayerPropertiesDialog
{
    Q_OBJECT
  public:
    KadasItemLayerProperties( KadasItemLayer *layer, QgsMapCanvas *canvas, QWidget *parent = nullptr );
};

#endif // KADASITEMLAYERPROPERTIES_H
