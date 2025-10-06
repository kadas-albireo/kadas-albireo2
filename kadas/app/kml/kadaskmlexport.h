/***************************************************************************
    kadaskmlexport.h
    ----------------
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

#ifndef KADASKMLEXPORT_H
#define KADASKMLEXPORT_H

#include <QList>
#include <QObject>

class QProgressDialog;
class QTextStream;
class QuaZip;
class QgsFeature;
class QgsFeatureRenderer;
class QgsLabelingEngine;
class QgsMapLayer;
class QgsMapSettings;
class QgsRectangle;
class QgsRenderContext;
class QgsVectorLayer;
class KadasKMLPalLabeling;

class KadasKMLExport : public QObject {
  Q_OBJECT
public:
  bool exportToFile(const QString &filename, const QList<QgsMapLayer *> &layers,
                    double exportScale,
                    const QgsCoordinateReferenceSystem &mapCrs,
                    const QgsRectangle &exportMapRect = QgsRectangle());
  static QString convertColor(const QColor &c);

private:
  void writeVectorLayerFeatures(QgsVectorLayer *vl, QTextStream &outStream,
                                QgsRenderContext &rc,
                                QgsLabelingEngine &labelingEngine,
                                const QgsRectangle &exportRect);
  void writeTiles(QgsMapLayer *mapLayer, const QgsRectangle &layerExtent,
                  double exportScale, QTextStream &outStream, int drawingOrder,
                  QuaZip *quaZip, QProgressDialog *progress);
  void writeGroundOverlay(QTextStream &outStream, const QString &name,
                          const QString &href, const QgsRectangle &latLongBox,
                          int drawingOrder);
  void writeMapItems(const QString &layerId, QTextStream &outStream,
                     QuaZip *quaZip);
  bool renderTile(QImage &img, const QgsRectangle &extent,
                  QgsMapLayer *mapLayer);
  void addStyle(QTextStream &outStream, QgsFeature &f, QgsFeatureRenderer &r,
                QgsRenderContext &rc);
};

#endif // KADASKMLEXPORT_H
