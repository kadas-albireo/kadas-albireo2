/***************************************************************************
    kadasapplication.cpp
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

#ifndef KADASAPPLICATION_H
#define KADASAPPLICATION_H

#include <QDateTime>

#include <qgis/qgis.h>
#include <qgis/qgsapplication.h>

#include <kadas/gui/kadasfeaturepicker.h>

class QgsLayerTreeMapCanvasBridge;
class QgsMapLayer;
class QgsMapTool;
class QgsRasterLayer;
class QgsVectorLayer;
class KadasClipboard;
class KadasMainWindow;
class KadasMapToolPan;

#define kApp KadasApplication::instance()

class KadasApplication : public QgsApplication
{
  Q_OBJECT

public:

  static KadasApplication *instance();

  KadasApplication(int &argc, char **argv);
  ~KadasApplication();

  KadasClipboard* clipboard() const{ return mClipboard; }
  KadasMainWindow* mainWindow() const{ return mMainWindow; }

  QgsRasterLayer *addRasterLayer(const QString &uri, const QString &baseName, const QString &providerKey) const;
  QgsVectorLayer* addVectorLayer(const QString &uri, const QString &layerName, const QString &providerKey) const;
  void addVectorLayers( const QStringList &layerUris, const QString &enc, const QString &dataSourceType )  const;
  void addMapLayers(const QList<QgsMapLayer*>& layers) const;
  void removeLayer(QgsMapLayer* layer) const;

  void exportToGpx();
  void exportToKml();
  void importFromGpx();
  void importFromKml();

  void paste();

  bool projectCreateFromTemplate(const QString& templateFile );
  bool projectOpen( const QString& projectFile = QString() );
  void projectClose();
  bool projectSave( const QString& fileName = QString(), bool promptFileName = false);

  void saveMapAsImage();
  void saveMapToClipboard();

  void showLayerAttributeTable(const QgsMapLayer* layer);
  void showLayerProperties(const QgsMapLayer* layer);
  void showLayerInfo(const QgsMapLayer* layer);

  QgsMapLayer* currentLayer() const;
  void refreshMapCanvas() const;

public slots:
  void displayMessage(const QString& message, Qgis::MessageLevel level = Qgis::Info);

signals:
  void projectRead();
  void activeLayerChanged(QgsMapLayer* layer);

private:
  KadasClipboard* mClipboard = nullptr;
  KadasMainWindow* mMainWindow = nullptr;
  QgsLayerTreeMapCanvasBridge *mLayerTreeCanvasBridge = nullptr;
  bool mBlockActiveLayerChanged = false;
  QDateTime mProjectLastModified;
  KadasMapToolPan* mMapToolPan = nullptr;

  QList<QgsMapLayer*> showGDALSublayerSelectionDialog(QgsRasterLayer *layer) const;
  QList<QgsMapLayer*> showOGRSublayerSelectionDialog(QgsVectorLayer *layer) const;
  bool showZipSublayerSelectionDialog(const QString& path) const;
  bool projectSaveDirty();

private slots:
  void onActiveLayerChanged( QgsMapLayer *layer );
  void onFocusChanged(QWidget* /*old*/, QWidget* now);
  void onMapToolChanged( QgsMapTool *newTool, QgsMapTool *oldTool );
  void handleItemPicked( const KadasFeaturePicker::PickResult& result );
  void showCanvasContextMenu( const QPoint& screenPos, const QgsPointXY& mapPos);
  void updateWindowTitle();
};

#endif // KADASAPPLICATION_H
