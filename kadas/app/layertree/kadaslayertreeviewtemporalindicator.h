/***************************************************************************
  kadaslayertreeviewtemporalindicator.cpp
  --------------------------------------
  Date                 : July 2024
  Copyright            : (C) 2024 by Damiano Lombardi
  Email                : damiano@opengis.ch
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef KADASLAYERTREEVIEWTEMPORALINDICATOR_H
#define KADASLAYERTREEVIEWTEMPORALINDICATOR_H


#include "external/qgis/app/qgslayertreeviewindicatorprovider.h"

class KadasTemporalController;

//! Adds indicators for showing temporal layers.
class KadasLayerTreeViewTemporalIndicator : public QgsLayerTreeViewIndicatorProvider
{
    Q_OBJECT
  public:
    explicit KadasLayerTreeViewTemporalIndicator( QgsLayerTreeView *view, KadasTemporalController *kadasTemporalController );

  protected:
    void connectSignals( QgsMapLayer *layer ) override;

  protected slots:

    void onIndicatorClicked( const QModelIndex &index ) override;

    //! Adds/removes indicator of a layer
    void onLayerChanged( QgsMapLayer *layer );

  private:
    bool acceptLayer( QgsMapLayer *layer ) override;
    QString iconName( QgsMapLayer *layer ) override;
    QString tooltipText( QgsMapLayer *layer ) override;

    KadasTemporalController *mKadasTemporalController;

};

#endif // KADASLAYERTREEVIEWTEMPORALINDICATOR_H
