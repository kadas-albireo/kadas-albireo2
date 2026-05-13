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

#include "kadas/gui/kadas_gui.h"

class QgsAnnotationLayer;
class QgsMapLayer;
class QgsMapCanvas;

class KADAS_GUI_EXPORT KadasFeaturePicker
{
  public:
    enum class PickObjective SIP_MONKEYPATCH_SCOPEENUM
    {
      PICK_OBJECTIVE_ANY,
      PICK_OBJECTIVE_TOOLTIP
    };

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
          annotationLayer = other.annotationLayer;
          annotationItemId = other.annotationItemId;
        }
        ~PickResult() { delete geom; }
        const PickResult &operator=( const PickResult &other )
        {
          layer = other.layer;
          geom = other.geom ? other.geom->clone() : nullptr;
          crs = other.crs;
          feature = other.feature;
          annotationLayer = other.annotationLayer;
          annotationItemId = other.annotationItemId;
          return *this;
        }
        bool isEmpty() const { return layer == nullptr && annotationLayer == nullptr; }

        QgsMapLayer *layer = nullptr;
        QgsAbstractGeometry *geom = nullptr;
        QgsCoordinateReferenceSystem crs;
        QgsFeature feature;

        //! When non-null, the pick hit a QgsAnnotationItem in this layer.
        QgsAnnotationLayer *annotationLayer = nullptr;
        //! Identifier of the picked annotation item within \a annotationLayer.
        QString annotationItemId;
    };

    static PickResult pick(
      const QgsMapCanvas *canvas,
      const QgsPointXY &mapPos,
      Qgis::GeometryType geomType = Qgis::GeometryType::Unknown,
      KadasFeaturePicker::PickObjective pickObjective = KadasFeaturePicker::PickObjective::PICK_OBJECTIVE_ANY
    );

  private:
    static PickResult pickAnnotationLayer( QgsAnnotationLayer *layer, const QgsMapCanvas *canvas, const QgsPointXY &mapPos );
    static PickResult pickVectorLayer( QgsVectorLayer *vlayer, const QgsMapCanvas *canvas, const QgsPointXY &mapPos, Qgis::GeometryType geomType );
};

#endif // KADASFEATUREPICKER_H
