/***************************************************************************
    kadas3dintegration.cpp
    -------------------------
    copyright            : (C) 2024 Matthias Kuhn
    email                : matthias@opengis.ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <QAction>
#include <QGuiApplication>
#include <QMessageBox>
#include <QScreen>
#include <QDomDocument>

#include <qgis/qgs3dmapcanvas.h>
#include <qgis/qgscameracontroller.h>

#include <qgs3d.h>
#include <qgs3dmapsettings.h>
#include <qgs3dutils.h>
#include <qgsdemterraingenerator.h>
#include <qgsmapcanvas.h>
#include <qgsproject.h>
#include <qgsrectangle.h>
#include <qgsprojectviewsettings.h>
#include <qgsdirectionallightsettings.h>
#include <qgsdockablewidgethelper.h>
#include <qgsmapviewsmanager.h>
#include <qgsrasterlayer.h>

#include "kadas/app/3d/kadas3dintegration.h"
#include "kadas/app/3d/kadas3dmapcanvaswidget.h"

static const QString KADAS_3D_IDENTIFIER = QStringLiteral( "Kadas 3D" );

void write3DMapViewSettings( Kadas3DMapCanvasWidget *widget, QDomDocument &doc, QDomElement &elem3DMap )
{
  QgsReadWriteContext readWriteContext;
  readWriteContext.setPathResolver( QgsProject::instance()->pathResolver() );
  elem3DMap.setAttribute( QStringLiteral( "name" ), widget->canvasName() );
  QDomElement elem3DMapSettings = widget->mapCanvas3D()->mapSettings()->writeXml( doc, readWriteContext );
  elem3DMap.appendChild( elem3DMapSettings );
  QDomElement elemCamera = widget->mapCanvas3D()->cameraController()->writeXml( doc );
  elem3DMap.appendChild( elemCamera );

  widget->dockableWidgetHelper()->writeXml( elem3DMap );
}

void read3DMapViewSettings( Kadas3DMapCanvasWidget *widget, QDomElement &elem3DMap )
{
  QgsReadWriteContext readWriteContext;
  readWriteContext.setPathResolver( QgsProject::instance()->pathResolver() );

  QDomElement elem3D = elem3DMap.firstChildElement( QStringLiteral( "qgis3d" ) );
  Qgs3DMapSettings *map = new Qgs3DMapSettings;
  map->readXml( elem3D, readWriteContext );
  map->resolveReferences( *QgsProject::instance() );

  map->setTransformContext( QgsProject::instance()->transformContext() );
  map->setPathResolver( QgsProject::instance()->pathResolver() );
  map->setMapThemeCollection( QgsProject::instance()->mapThemeCollection() );
  QObject::connect( QgsProject::instance(), &QgsProject::transformContextChanged, map, [map] {
    map->setTransformContext( QgsProject::instance()->transformContext() );
  } );

#if 0
  // these things are not saved in project
  map->setSelectionColor( mMapCanvas->selectionColor() );
  map->setBackgroundColor( mMapCanvas->canvasColor() );
#endif
  map->setOutputDpi( QGuiApplication::primaryScreen()->logicalDotsPerInch() );

  widget->setMapSettings( map );

  QDomElement elemCamera = elem3DMap.firstChildElement( QStringLiteral( "camera" ) );
  if ( !elemCamera.isNull() )
  {
    widget->mapCanvas3D()->cameraController()->readXml( elemCamera );
  }

  widget->dockableWidgetHelper()->readXml( elem3DMap );
}

Kadas3DIntegration::Kadas3DIntegration( QAction *action3D, QgsMapCanvas *mapCanvas, QObject *parent )
  : QObject( parent ), mAction3D( action3D ), mMapCanvas( mapCanvas )
{
  Qgs3D::initialize();

  connect( QgsProject::instance(), &QgsProject::readProject, [this] {
    if ( m3DMapCanvasWidget )
    {
      QDomElement viewConfig = QgsProject::instance()->viewsManager()->get3DViewSettings( KADAS_3D_IDENTIFIER );
      read3DMapViewSettings( m3DMapCanvasWidget, viewConfig );
    }
  } );

  connect( mAction3D, &QAction::triggered, [this]() {
    if ( !m3DMapCanvasWidget )
    {
      if ( !QgsProject::instance()->viewsManager()->get3DViewSettings( KADAS_3D_IDENTIFIER ).isNull() )
      {
        m3DMapCanvasWidget = new Kadas3DMapCanvasWidget( KADAS_3D_IDENTIFIER, true );
        m3DMapCanvasWidget->setMainCanvas( mMapCanvas );

        QDomElement viewConfig = QgsProject::instance()->viewsManager()->get3DViewSettings( KADAS_3D_IDENTIFIER );
        read3DMapViewSettings( m3DMapCanvasWidget, viewConfig );
      }
      else
      {
        m3DMapCanvasWidget = createNewMapCanvas3D( KADAS_3D_IDENTIFIER );
      }

      if ( m3DMapCanvasWidget )
      {
        connect( m3DMapCanvasWidget->mapCanvas3D()->mapSettings(), &Qgs3DMapSettings::settingsChanged, [this]() {
          QDomImplementation DomImplementation;
          QDomDocumentType documentType = DomImplementation.createDocumentType(
            QStringLiteral( "qgis" ), QStringLiteral( "http://mrcc.com/qgis.dtd" ), QStringLiteral( "SYSTEM" )
          );
          QDomDocument doc( documentType );

          QDomElement elem3DMap;
          elem3DMap = doc.createElement( QStringLiteral( "view" ) );
          write3DMapViewSettings( m3DMapCanvasWidget, doc, elem3DMap );

          QgsProject::instance()->viewsManager()->register3DViewSettings( KADAS_3D_IDENTIFIER, elem3DMap );
        } );

        connect( m3DMapCanvasWidget->dockableWidgetHelper(), &QgsDockableWidgetHelper::closed, [this]() {
          m3DMapCanvasWidget->deleteLater();
          m3DMapCanvasWidget = nullptr;
          mAction3D->setChecked( false );
        } );
      }
      else
      {
        // happens e.g. when the project extent is unknown. Keep the button unchecked.
        mAction3D->setChecked( false );
      }
    }
    else
    {
      m3DMapCanvasWidget->close();
    }
  } );
}

Kadas3DMapCanvasWidget *Kadas3DIntegration::createNewMapCanvas3D( const QString &name )
{
  // initialize from project
  const QgsRectangle fullExtent = mMapCanvas->projectExtent();

  // some layers may go crazy and make full extent unusable
  // we can't go any further - invalid extent would break everything
  if ( fullExtent.isEmpty() || !fullExtent.isFinite() )
  {
    QMessageBox::warning( mMapCanvas, tr( "New 3D Map View" ), tr( "Project extent is not valid. Please add or activate a layer to render." ) );
    return nullptr;
  }

  Kadas3DMapCanvasWidget *canvasWidget = new Kadas3DMapCanvasWidget( name, true );
  canvasWidget->setMainCanvas( mMapCanvas );

  QgsProject *prj = QgsProject::instance();
  QgsSettings settings;

  Qgs3DMapSettings *map = new Qgs3DMapSettings;
  if ( !prj->crs().isGeographic() )
  {
    map->setCrs( prj->crs() );
  }
  else
  {
    map->setCrs( QgsCoordinateReferenceSystem( QStringLiteral( "EPSG:3857" ) ) );
  }

  const QgsReferencedRectangle projectExtent = prj->viewSettings()->fullExtent();
  const QgsRectangle fullExtent3d = Qgs3DUtils::tryReprojectExtent2D( projectExtent, projectExtent.crs(), map->crs(), prj->transformContext() );
  map->setOrigin( QgsVector3D( fullExtent3d.center().x(), fullExtent3d.center().y(), 0 ) );
  map->setSelectionColor( mMapCanvas->selectionColor() );
  map->setBackgroundColor( mMapCanvas->canvasColor() );
  map->setLayers( mMapCanvas->layers( true ) );
  map->setTemporalRange( mMapCanvas->temporalRange() );

  const Qgis::NavigationMode defaultNavMode = settings.enumValue( QStringLiteral( "map3d/defaultNavigation" ), Qgis::NavigationMode::TerrainBased, QgsSettings::App );
  map->setCameraNavigationMode( defaultNavMode );

  map->setCameraMovementSpeed( settings.value( QStringLiteral( "map3d/defaultMovementSpeed" ), 5, QgsSettings::App ).toDouble() );
  const Qt3DRender::QCameraLens::ProjectionType defaultProjection = settings.enumValue( QStringLiteral( "map3d/defaultProjection" ), Qt3DRender::QCameraLens::PerspectiveProjection, QgsSettings::App );
  map->setProjectionType( defaultProjection );
  map->setFieldOfView( settings.value( QStringLiteral( "map3d/defaultFieldOfView" ), 45, QgsSettings::App ).toInt() );

  map->setTransformContext( QgsProject::instance()->transformContext() );
  map->setPathResolver( QgsProject::instance()->pathResolver() );
  map->setMapThemeCollection( QgsProject::instance()->mapThemeCollection() );

  map->configureTerrainFromProject( QgsProject::instance()->elevationProperties(), fullExtent3d );

  // new scenes default to a single directional light
  map->setLightSources( QList<QgsLightSource *>() << new QgsDirectionalLightSettings() );
  map->setOutputDpi( QGuiApplication::primaryScreen()->logicalDotsPerInch() );
  map->setRendererUsage( Qgis::RendererUsage::View );

  connect( QgsProject::instance(), &QgsProject::transformContextChanged, map, [map] {
    map->setTransformContext( QgsProject::instance()->transformContext() );
  } );

  canvasWidget->setMapSettings( map );

  const QgsRectangle canvasExtent = Qgs3DUtils::tryReprojectExtent2D( mMapCanvas->extent(), mMapCanvas->mapSettings().destinationCrs(), map->crs(), prj->transformContext() );
  float dist = static_cast<float>( std::max( canvasExtent.width(), canvasExtent.height() ) );
  canvasWidget->mapCanvas3D()->setViewFromTop( canvasExtent.center(), dist, static_cast<float>( mMapCanvas->rotation() ) );

  const Qgis::VerticalAxisInversion axisInversion = settings.enumValue( QStringLiteral( "map3d/axisInversion" ), Qgis::VerticalAxisInversion::WhenDragging, QgsSettings::App );
  if ( canvasWidget->mapCanvas3D()->cameraController() )
    canvasWidget->mapCanvas3D()->cameraController()->setVerticalAxisInversion( axisInversion );


  QString heightLayerId = QgsProject::instance()->readEntry( "Heightmap", "layer" );
  QgsRasterLayer *heightLayer = qobject_cast<QgsRasterLayer *>( QgsProject::instance()->mapLayer( heightLayerId ) );
  if ( heightLayer )
  {
    map->setTerrainRenderingEnabled( true );
    QgsDemTerrainGenerator *demTerrainGen = new QgsDemTerrainGenerator;
    demTerrainGen->setCrs( map->crs(), QgsProject::instance()->transformContext() );
    demTerrainGen->setLayer( heightLayer );
    demTerrainGen->setResolution( 16 );
    demTerrainGen->setSkirtHeight( 1000 );
    map->setTerrainGenerator( demTerrainGen );
  }

  return canvasWidget;
}
