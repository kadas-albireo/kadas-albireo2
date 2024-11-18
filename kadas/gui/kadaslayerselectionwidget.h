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

#include "kadas/gui/kadas_gui.h"

class QComboBox;
class QgsLayerTreeView;
class QgsMapCanvas;
class QgsMapLayer;

class KADAS_GUI_EXPORT KadasLayerSelectionWidget : public QWidget
{
    Q_OBJECT
  public:
#ifndef SIP_RUN
    typedef std::function<bool( QgsMapLayer * )> LayerFilter;
    typedef std::function<QgsMapLayer *( const QString & )> LayerCreator;

    KadasLayerSelectionWidget( QgsMapCanvas *canvas, QgsLayerTreeView *layerTreeView, LayerFilter filter = nullptr, LayerCreator creator = nullptr, QWidget *parent = nullptr );
    // clang-format off
#else
    KadasLayerSelectionWidget( QgsMapCanvas *canvas, QgsLayerTreeView *layerTreeView, SIP_PYCALLABLE filter, SIP_PYCALLABLE creator, QWidget *parent = nullptr )[( QgsMapCanvas *, QgsLayerTreeView *, LayerFilter, LayerCreator, QWidget * )];
    % MethodCode

    // Make sure the callables doesn't get garbage collected, this is needed because refcount for a1/a2 is 0
    // and the creation function pointer is passed to the metadata and it needs to be kept in memory.
    Py_INCREF( a2 );
    Py_INCREF( a3 );

    Py_BEGIN_ALLOW_THREADS

    auto layerFilter = [a2]( QgsMapLayer *layer ) -> bool
    {
      bool result = false;
      SIP_BLOCK_THREADS
      PyObject *s = sipCallMethod( NULL, a2, "D", layer, sipType_QgsMapLayer, NULL );
      if ( s )
      {
        result = sipConvertToBool( s );
      }
      SIP_UNBLOCK_THREADS
      return result;
    };

    auto layerCreator = [a3]( const QString &name ) -> QgsMapLayer *
    {
      QgsMapLayer *result = nullptr;
      SIP_BLOCK_THREADS
      PyObject *s = sipCallMethod( NULL, a3, "D", &name, sipType_QString, NULL );
      if ( s )
      {
        int state;
        int sipIsError = 0;
        result = reinterpret_cast<QgsMapLayer *>( sipConvertToType( s, sipType_QgsMapLayer, NULL, SIP_NOT_NONE, &state, &sipIsError ) );
        sipReleaseType( result, sipType_QgsMapLayer, state );
      }
      SIP_UNBLOCK_THREADS
      return result;
    };

    try
    {
      sipCpp = new sipKadasLayerSelectionWidget( a0, a1, layerFilter, layerCreator, a4 );
    }
    catch ( ... )
    {
      Py_BLOCK_THREADS

      sipRaiseUnknownException();
      return SIP_NULLPTR;
    }
    Py_END_ALLOW_THREADS

    % End
#endif
    // clang-format on

    KadasLayerSelectionWidget( QgsMapCanvas *canvas, QgsLayerTreeView *layerTreeView, QWidget *parent = nullptr )
      : KadasLayerSelectionWidget( canvas, layerTreeView, nullptr, nullptr, parent ) {}
    void createLayerIfEmpty( const QString &name );
    void setLabel( const QString &label );
    QgsMapLayer *getSelectedLayer() const;

  public slots:
    void setSelectedLayer( QgsMapLayer *layer );

  signals:
    void selectedLayerChanged( QgsMapLayer *layer );

  private:
    QgsMapCanvas *mCanvas = nullptr;
    QgsLayerTreeView *mLayerTreeView = nullptr;
    QLabel *mLabel = nullptr;
    QComboBox *mLayersCombo = nullptr;

    LayerFilter mFilter = nullptr;
    LayerCreator mCreator = nullptr;

  private slots:
    void createLayer();
    void layerSelectionChanged( int idx );
    void repopulateLayers();
};

#endif // KADASLAYERSELECTIONWIDGET_H
