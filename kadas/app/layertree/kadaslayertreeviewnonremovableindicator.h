/***************************************************************************
  qgslayertreeviewnonremovableindicator.h
  --------------------------------------
  Date                 : July 2026
  Copyright            : (C) 2026 by Valentin Buira
  Email                : valentin dot buira at opengis dot ch
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef KADASLAYERTREEVIEWNONREMOVABLEINDICATOR_H
#define KADASLAYERTREEVIEWNONREMOVABLEINDICATOR_H

#include <memory>

#include "external/qgis/app/qgslayertreeviewindicatorprovider.h"


#include <QSet>

/**
 * Forked from https://github.com/ValentinBuira/QGIS/blob/8c9806d/src/app/qgslayertreeviewnonremovableindicator.cpp 
 */
class KadasLayerTreeViewNonRemovableIndicatorProvider : public QgsLayerTreeViewIndicatorProvider
{
    Q_OBJECT
  public:
    explicit KadasLayerTreeViewNonRemovableIndicatorProvider( QgsLayerTreeView *view );

  private:
    QString iconName( QgsMapLayer *layer ) override;
    QString tooltipText( QgsMapLayer *layer ) override;
    bool acceptLayer( QgsMapLayer *layer ) override;

  protected:
    void connectSignals( QgsMapLayer *layer ) override;
    void disconnectSignals( QgsMapLayer *layer ) override;
};


#endif // KADASLAYERTREEVIEWNONREMOVABLEINDICATOR_H
