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

#include <QApplication>

class QgsMapLayer;
class QgsMapTool;
class KadasClipboard;
class KadasMainWindow;

#define kApp KadasApplication::instance()

class KadasApplication : public QApplication
{
  Q_OBJECT

public:

  static KadasApplication *instance();

  KadasApplication(int &argc, char **argv);

  KadasClipboard* clipboard() const{ return mClipboard; }
  KadasMainWindow* mainWindow() const{ return mMainWindow; }

  void addDelimitedTextLayer();
  void addRasterLayer();
  void addVectorLayer();
  void addWcsLayer();
  void addWfsLayer();
  void addWmsLayer();

  void exportToGpx();
  void exportToKml();
  void importFromGpx();
  void importFromKml();

  QgsMapTool* mapToolBullsEye() const{ return nullptr; } // TODO
  QgsMapTool* mapToolDeleteItems() const{ return nullptr; } // TODO
  QgsMapTool* mapToolGuideGrid() const{ return nullptr; } // TODO
  QgsMapTool* mapToolHeightProfile() const{ return nullptr; } // TODO
  QgsMapTool* mapToolHillshade() const{ return nullptr; } // TODO
  QgsMapTool* mapToolImageAnnotation() const{ return nullptr; } // TODO
  QgsMapTool* mapToolMeasureAzimuth() const{ return nullptr; } // TODO
  QgsMapTool* mapToolMeasureArea() const{ return nullptr; } // TODO
  QgsMapTool* mapToolMeasureCircle() const{ return nullptr; } // TODO
  QgsMapTool* mapToolMeasureDistance() const{ return nullptr; } // TODO
  QgsMapTool* mapToolSlope() const{ return nullptr; } // TODO
  QgsMapTool* mapToolPan() const{ return nullptr; } // TODO
  QgsMapTool* mapToolPinAnnotation() const{ return nullptr; } // TODO
  QgsMapTool* mapToolSvgAnnotation() const{ return nullptr; } // TODO
  QgsMapTool* mapToolViewshed() const{ return nullptr; } // TODO

  void paste();

  void projectOpen( const QString& fileName );
  void projectSave();
  void projectSaveAs( const QString& fileName = QString() );

  void saveMapAsImage();
  void saveMapToClipboard();

  void showLayerAttributeTable(const QgsMapLayer* layer);
  void showLayerProperties(const QgsMapLayer* layer);
  void showLayerInfo(const QgsMapLayer* layer);

  void zoomFull();
  void zoomIn();
  void zoomNext();
  void zoomOut();
  void zoomPrev();

private:
  KadasClipboard* mClipboard = nullptr;
  KadasMainWindow* mMainWindow = nullptr;
};

#endif // KADASAPPLICATION_H
