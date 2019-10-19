/***************************************************************************
    kadaslayerselectionwidget.h
    ---------------------------
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

#ifndef KADASLAYERSELECTIONWIDGET_H
#define KADASLAYERSELECTIONWIDGET_H

#include <functional>

#include <QLabel>
#include <QWidget>

#include <kadas/gui/kadas_gui.h>

class QComboBox;
class QgsMapCanvas;
class QgsMapLayer;

class KADAS_GUI_EXPORT KadasLayerSelectionWidget : public QWidget
{
    Q_OBJECT
  public:
#ifndef SIP_RUN
    typedef std::function<bool( QgsMapLayer * )> LayerFilter;
    typedef std::function<QgsMapLayer*( const QString & )> LayerCreator;

    KadasLayerSelectionWidget( QgsMapCanvas *canvas, LayerFilter filter = nullptr, LayerCreator creator = nullptr, QWidget *parent = nullptr );
#endif
    KadasLayerSelectionWidget( QgsMapCanvas *canvas, QWidget *parent = nullptr ) : KadasLayerSelectionWidget( canvas, nullptr, nullptr, parent ) {}
    void setLabel( const QString &label );
    QgsMapLayer *getSelectedLayer() const;

  public slots:
    void setSelectedLayer( QgsMapLayer *layer );

  signals:
    void selectedLayerChanged( QgsMapLayer *layer );

  private:
    LayerFilter mFilter = nullptr;
    LayerCreator mCreator = nullptr;

    QgsMapCanvas *mCanvas;
    QLabel *mLabel;
    QComboBox *mLayersCombo;

  private slots:
    void createLayer();
    void setSelectedLayer( int idx );
    void repopulateLayers();
};

#endif // KADASLAYERSELECTIONWIDGET_H
