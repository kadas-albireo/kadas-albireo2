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

class QgsMapLayer;
class QgsMapCanvas;
class QgsMapCanvasAnnotationItem;

class KADAS_GUI_EXPORT KadasFeaturePicker
{
  public:
    class PickResult
    {
      public:
        PickResult() : layer( 0 ), annotation( 0 ) {}
        bool isEmpty() const { return layer == 0 && annotation == 0; }

        QgsMapLayer* layer;
        QgsFeature feature;
        QVariant otherResult;
        QgsMapCanvasAnnotationItem* annotation;
        QgsLabelPosition labelPos;
        QRectF boundingBox;
    };

    typedef bool( *filter_t )( const QgsFeature& );
    static PickResult pick(const QgsMapCanvas *canvas, const QPoint &canvasPos, const QgsPointXY &mapPos, QgsWkbTypes::GeometryType geomType, filter_t filter = 0 );
};

#endif // KADASFEATUREPICKER_H

