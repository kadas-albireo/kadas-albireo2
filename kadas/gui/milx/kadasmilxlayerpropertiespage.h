/***************************************************************************
    kadasmilxlayerpropertiespage.h
    ------------------------------
    copyright            : (C) 2021 by Sandro Mani
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

#ifndef KADASMILXLAYERPROPERTIESPAGE_H
#define KADASMILXLAYERPROPERTIESPAGE_H

#include <QIcon>

#include <qgis/qgsmaplayerconfigwidget.h>
#include <qgis/qgsmaplayerconfigwidgetfactory.h>

#include "kadas/gui/kadas_gui.h"

class QDomDocument;
class QDomElement;
class QComboBox;
class QGroupBox;
class QSlider;
class QSpinBox;
class QgsColorButton;
class QgsMapLayer;
class KadasMilxLayer;

class KADAS_GUI_EXPORT KadasMilxLayerPropertiesPage
    : public QgsMapLayerConfigWidget {
  Q_OBJECT

public:
  explicit KadasMilxLayerPropertiesPage(KadasMilxLayer *layer,
                                        QgsMapCanvas *canvas,
                                        QWidget *parent = nullptr);

public slots:
  void apply() override;

private:
  KadasMilxLayer *mLayer = nullptr;
  QGroupBox *mGroupBox = nullptr;
  QSlider *mSymbolSizeSlider = nullptr;
  QSlider *mLineWidthSlider = nullptr;
  QComboBox *mWorkModeCombo = nullptr;
  QSpinBox *mLeaderLineWidthSpin = nullptr;
  QgsColorButton *mLeaderLineColorButton = nullptr;
};

class KADAS_GUI_EXPORT KadasMilxLayerPropertiesPageFactory
    : public QObject,
      public QgsMapLayerConfigWidgetFactory {
  Q_OBJECT
public:
  explicit KadasMilxLayerPropertiesPageFactory(QObject *parent = nullptr);
  QgsMapLayerConfigWidget *createWidget(QgsMapLayer *layer,
                                        QgsMapCanvas *canvas, bool dockWidget,
                                        QWidget *parent) const override;
  QIcon icon() const override;
  QString title() const override;
  bool supportLayerPropertiesDialog() const override { return true; }
  bool supportsLayer(QgsMapLayer *layer) const override;

private slots:
  void readLayerConfig(QgsMapLayer *mapLayer, const QDomElement &elem);
  void writeLayerConfig(QgsMapLayer *mapLayer, QDomElement &elem,
                        QDomDocument &doc);
};

#endif // KADASGLOBEVECTORLAYERPROPERTIES_H
