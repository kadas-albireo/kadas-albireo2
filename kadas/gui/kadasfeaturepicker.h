/***************************************************************************
    kadasfeaturepicker.h
    --------------------
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

#ifndef KADASFEATUREPICKER_H
#define KADASFEATUREPICKER_H

#include <QRectF>

#include <qgis/qgsfeature.h>
#include <qgis/qgspallabeling.h>
#include <qgis/qgswkbtypes.h>

#include <kadas/gui/kadas_gui.h>
#include <kadas/gui/kadasitemlayer.h>

class QgsMapLayer;
class QgsMapCanvas;

class KADAS_GUI_EXPORT KadasFeaturePicker
{
  public:
    class PickResult
    {
      public:
        PickResult() = default;
        PickResult( const PickResult &other )
        {
          layer = other.layer;
          geom = other.geom ? other.geom->clone() : nullptr;
          crs = other.crs;
          feature = other.feature;
          itemId = other.itemId;
        }
        ~PickResult() { delete geom; }
        const PickResult &operator=( const PickResult &other )
        {
          layer = other.layer;
          geom = other.geom ? other.geom->clone() : nullptr;
          crs = other.crs;
          feature = other.feature;
          itemId = other.itemId;
          return *this;
        }
        bool isEmpty() const { return layer == nullptr; }

        QgsMapLayer *layer = nullptr;
        QgsAbstractGeometry *geom = nullptr;
        QgsCoordinateReferenceSystem crs;
        QgsFeature feature;
        KadasItemLayer::ItemId itemId = KadasItemLayer::ITEM_ID_NULL;
    };

    static PickResult pick( const QgsMapCanvas *canvas, const QgsPointXY &mapPos, QgsWkbTypes::GeometryType geomType = QgsWkbTypes::UnknownGeometry, KadasItemLayer::PickObjective pickObjective = KadasItemLayer::PICK_OBJECTIVE_ANY );
#ifndef SIP_RUN
    [[deprecated( "Use variant without canvasPos instead" )]]
#endif
    static PickResult pick( const QgsMapCanvas *canvas, const QPoint &canvasPos, const QgsPointXY &mapPos, QgsWkbTypes::GeometryType geomType );

  private:
    static PickResult pickItemLayer( KadasItemLayer *layer, const QgsMapCanvas *canvas, const KadasMapPos &mapPos, KadasItemLayer::PickObjective pickObjective );
    static PickResult pickVectorLayer( QgsVectorLayer *vlayer, const QgsMapCanvas *canvas, const QgsPointXY &mapPos, QgsWkbTypes::GeometryType geomType );
};

#endif // KADASFEATUREPICKER_H

