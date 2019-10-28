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
class QgsMapLayer;
class QgsPointXY;
class QgsRectangle;
class QgsVectorLayer;
class KadasGlobeFrustumHighlightCallback;
class KadasGlobeLayerPropertiesFactory;
class KadasGlobeTileSource;
class KadasGlobeVectorLayerConfig;
class KadasGlobeWidget;

namespace osg { class Group; }
namespace osgViewer { class Viewer; }

namespace osgEarth
{
  class GeoPoint;
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

    //! Enable or disable frustum highlight
    void enableFrustumHighlight( bool statu );

    //! emits signal with current mouse coordinates
    void showCurrentCoordinates( const osgEarth::GeoPoint &geoPoint );

    //! Gets the OSG viewer
    osgViewer::Viewer *osgViewer() { return mOsgViewer; }
    //! Gets OSG map node
    osgEarth::MapNode *mapNode() { return mMapNode; }

  public slots:
    void run();
    void updateLayers();
    void showSettings();
    void syncExtent();

  private:
    osgEarth::QtGui::ViewerWidget *mViewerWidget = nullptr;
    KadasGlobeWidget *mDockWidget = nullptr;
    KadasGlobeDialog *mSettingsDialog = nullptr;

    QString mBaseLayerUrl;
    QList<KadasGlobeDialog::LayerDataSource> mImagerySources;
    QList<KadasGlobeDialog::LayerDataSource> mElevationSources;

    osg::ref_ptr<osgViewer::Viewer> mOsgViewer;
    osg::ref_ptr<osgEarth::MapNode> mMapNode;
    osg::ref_ptr<osg::Group> mRootNode;
    osg::ref_ptr<osgEarth::Util::SkyNode> mSkyNode;
    osg::ref_ptr<osgEarth::ImageLayer> mBaseLayer;
    osg::ref_ptr<osgEarth::ImageLayer> mQgisMapLayer;
    osg::ref_ptr<KadasGlobeTileSource> mTileSource;
    QMap<QString, QgsRectangle> mLayerExtents;
    osg::ref_ptr<osgEarth::Util::VerticalScale> mVerticalScale;

    //! Creates additional pages in the layer properties for adjusting 3D properties
    KadasGlobeLayerPropertiesFactory *mLayerPropertiesFactory = nullptr;
    osg::ref_ptr<KadasGlobeFrustumHighlightCallback> mFrustumHighlightCallback;
    osg::ref_ptr<osgEarth::Util::Controls::LabelControl> mStatsLabel;

    void setupProxy();
    void addControl( osgEarth::Util::Controls::Control *control, int x, int y, int w, int h, osgEarth::Util::Controls::ControlEventHandler *handler );
    void addImageControl( const std::string &imgPath, int x, int y, osgEarth::Util::Controls::ControlEventHandler *handler = 0 );
    void addModelLayer( QgsVectorLayer *mapLayer, KadasGlobeVectorLayerConfig *layerConfig );
    void setupControls();
    void applyProjectSettings();
    QgsRectangle getQGISLayerExtent() const;

  private slots:
    void setGlobeEnabled( bool enabled );
    void reset();
    void projectRead();
    void applySettings();
    void layerChanged();
    void rebuildQGISLayer();
    void refreshQGISMapLayer( const QgsRectangle &dirtyRect );
    void updateTileStats( int queued, int tot );

  signals:
    //! emits current mouse position
    void xyCoordinates( const QgsPointXY &p );
};

#endif // KADASGLOBEINTEGRATION_H
