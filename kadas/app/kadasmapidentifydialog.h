/***************************************************************************
    kadasmapidentifydialog.h
    ------------------------
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

#ifndef KADASMAPIDENTIFYDIALOG_H
#define KADASMAPIDENTIFYDIALOG_H

#include <QDialog>
#include <QMap>

#include <kadas/core/kadaspluginlayer.h>

class QNetworkReply;
class QTreeWidget;
class QTreeWidgetItem;
class QgsAbstractGeometry;
class QgsFeature;
class QgsGeometryRubberBand;
class QgsMapCanvas;
class QgsPinAnnotationItem;
class QgsRasterIdentifyResult;
class QgsRasterLayer;
class QgsVectorLayer;
class KadasPinItem;


class KadasMapIdentifyDialog : public QDialog
{
    Q_OBJECT

  public:
    static void popup( QgsMapCanvas *canvas, const QgsPointXY &mapPos );

  private:
    KadasMapIdentifyDialog( QgsMapCanvas *canvas, const QgsPointXY &mapPos );
    ~KadasMapIdentifyDialog();

    static const int sGeometryRole;
    static const int sGeometryCrsRole;
    static QPointer<KadasMapIdentifyDialog> sInstance;

    QgsMapCanvas *mCanvas = nullptr;
    QTreeWidget *mTreeWidget = nullptr;
    QgsGeometryRubberBand *mRubberband = nullptr;
    KadasPinItem *mClickPosPin = nullptr;
    KadasPinItem *mResultPin = nullptr;
    QList<QgsAbstractGeometry *> mGeometries;
    QTimer *mTimeoutTimer = nullptr;
    QNetworkReply *mRasterIdentifyReply = nullptr;
    QMap<QString, QTreeWidgetItem *> mLayerTreeItemMap;

    void collectInfo( const QgsPointXY &mapPos );
    void addPluginLayerResults( KadasPluginLayer *pLayer, const QList<KadasPluginLayer::IdentifyResult> &results );
    void addVectorLayerResult( QgsVectorLayer *vLayer, const QgsFeature &feature );
    void addRasterIdentifyResult( QgsRasterLayer *rLayer, const QgsRasterIdentifyResult &result );

  private slots:
    void clear();
    void onItemClicked( QTreeWidgetItem *item, int /*col*/ );
    void rasterIdentifyFinished();
};

#endif // KADASMAPIDENTIFYDIALOG_H
