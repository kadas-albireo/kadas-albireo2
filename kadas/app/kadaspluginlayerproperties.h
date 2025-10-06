/***************************************************************************
    kadaspluginlayerproperties.h
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

#ifndef KADASPLUGINLAYERPROPERTIES_H
#define KADASPLUGINLAYERPROPERTIES_H

#include <qgis/qgsmaplayerconfigwidget.h>

#include "kadas/gui/kadaslayerpropertiesdialog.h"

class QGroupBox;
class QgsScaleRangeWidget;
class KadasPluginLayer;

class KadasPluginLayerRenderingPropertiesWidget
    : public QgsMapLayerConfigWidget {
  Q_OBJECT

public:
  explicit KadasPluginLayerRenderingPropertiesWidget(KadasPluginLayer *layer,
                                                     QgsMapCanvas *canvas,
                                                     QWidget *parent = nullptr);

public slots:
  void apply() override;

private:
  QGroupBox *mGroupBox = nullptr;
  QgsScaleRangeWidget *mScaleRangeWidget = nullptr;
};

class KadasPluginLayerProperties : public KadasLayerPropertiesDialog {
  Q_OBJECT
public:
  KadasPluginLayerProperties(KadasPluginLayer *layer, QgsMapCanvas *canvas,
                             QWidget *parent = nullptr);
};

#endif // KADASPLUGINLAYERPROPERTIES_H
