/***************************************************************************
    kadasglobeintegration.h
    -----------------------
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

#ifndef KADASGLOBEINTEGRATION_H
#define KADASGLOBEINTEGRATION_H

#include <QObject>
#include <osg/ref_ptr>
#include <osgEarth/Version>

#include <kadas/app/globe/kadasglobedialog.h>

class QAction;
class QgsPointXY;
class KadasGlobeLayerPropertiesFactory;
class KadasGlobeProjectLayerManager;
class KadasGlobeWidget;

namespace osg { class Group; }
namespace osgViewer { class Viewer; }

namespace osgEarth
{
  class ImageLayer;
  class MapNode;
  namespace QtGui { class ViewerWidget; }
  namespace Util
  {
    class SkyNode;
    class VerticalScale;
    namespace Controls
    {
      class Control;
      class ControlEventHandler;
      class LabelControl;
    }
  }
}


class KadasGlobeIntegration : public QObject
{
    Q_OBJECT

  public:
    KadasGlobeIntegration( QAction *action3D, QObject *parent = nullptr );
    ~KadasGlobeIntegration();

    osgEarth::MapNode *mapNode() { return mMapNode; }
    osgViewer::Viewer *osgViewer() { return mOsgViewer; }
    void showCurrentCoordinates( double lon, double lat );

  public slots:
    void syncExtent();

  signals:
    void xyCoordinates( const QgsPointXY &p );

  private:
    QAction *mAction3D = nullptr;
    osgEarth::QtGui::ViewerWidget *mViewerWidget = nullptr;
    KadasGlobeWidget *mDockWidget = nullptr;
    KadasGlobeDialog *mSettingsDialog = nullptr;
    KadasGlobeLayerPropertiesFactory *mLayerPropertiesFactory = nullptr;
    KadasGlobeProjectLayerManager *mProjectLayerManager = nullptr;

    QList<KadasGlobeDialog::LayerDataSource> mImagerySources;
    QList<KadasGlobeDialog::LayerDataSource> mElevationSources;

    osg::ref_ptr<osgViewer::Viewer> mOsgViewer;
    osg::ref_ptr<osgEarth::MapNode> mMapNode;
    osg::ref_ptr<osg::Group> mRootNode;
    osg::ref_ptr<osgEarth::Util::SkyNode> mSkyNode;
    osg::ref_ptr<osgEarth::ImageLayer> mBaseLayer;
    osg::ref_ptr<osgEarth::Util::VerticalScale> mVerticalScale;
    osg::ref_ptr<osgEarth::Util::Controls::LabelControl> mStatsLabel;

    void addControl( osgEarth::Util::Controls::Control *control, int x, int y, int w, int h, osgEarth::Util::Controls::ControlEventHandler *handler );
    void addImageControl( const std::string &imgPath, int x, int y, osgEarth::Util::Controls::ControlEventHandler *handler = 0 );
    void applyProjectSettings();
    void setupControls();
    void setupProxy();

  private slots:
    void applySettings();
    void projectRead();
    void reset();
    void run();
    void setGlobeEnabled( bool enabled );
    void showSettings();
    void updateTileStats( int queued, int tot );
};

#endif // KADASGLOBEINTEGRATION_H
