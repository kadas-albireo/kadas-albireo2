/***************************************************************************
    kadasgpxintegration.cpp
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

#include <QAction>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QShortcut>

#include <qgis/qgsannotationlayer.h>
#include <qgis/qgscoordinatereferencesystem.h>
#include <qgis/qgslinestring.h>
#include <qgis/qgsmessagebar.h>
#include <qgis/qgsmultilinestring.h>
#include <qgis/qgsmultipoint.h>
#include <qgis/qgsproject.h>

#include "kadas/gui/annotationitems/kadasannotationcontrollerregistry.h"
#include "kadas/gui/annotationitems/kadasannotationitemcontroller.h"
#include "kadas/gui/annotationitems/kadasannotationlayerhelpers.h"
#include "kadas/gui/annotationitems/kadasannotationlayerregistry.h"
#include "kadas/gui/annotationitems/kadasgpxrouteannotationitem.h"
#include "kadas/gui/annotationitems/kadasgpxwaypointannotationitem.h"
#include "kadas/gui/kadasitemlayer.h"
#include "kadas/gui/kadaslayerselectionwidget.h"
#include "kadas/gui/mapitemeditors/kadasgpxrouteeditor.h"
#include "kadas/gui/mapitemeditors/kadasgpxwaypointeditor.h"
#include "kadas/gui/mapitems/kadasgpxrouteitem.h"
#include "kadas/gui/mapitems/kadasgpxwaypointitem.h"
#include "kadas/gui/mapitems/kadaslineitem.h"
#include "kadas/gui/maptools/kadasmaptoolcreateannotationitem.h"
#include "kadas/gui/maptools/kadasmaptoolcreateitem.h"

#include "kadasapplication.h"
#include "kadasgpxintegration.h"
#include "kadasmainwindow.h"


KadasGpxIntegration::KadasGpxIntegration( QAction *actionWaypoint, QAction *actionRoute, QAction *actionExportGpx, QAction *actionImportGpx, QObject *parent )
  : QObject( parent )
{
  connect( actionWaypoint, &QAction::triggered, this, [this]( bool active ) { toggleAnnotation( active, Variant::Waypoint ); } );
  connect( actionRoute, &QAction::triggered, this, [this]( bool active ) { toggleAnnotation( active, Variant::Route ); } );
  connect( actionExportGpx, &QAction::triggered, this, &KadasGpxIntegration::saveGpx );
  connect( actionImportGpx, &QAction::triggered, this, &KadasGpxIntegration::openGpx );

  kApp->mainWindow()->addCustomDropHandler( &mDropHandler );
}

KadasGpxIntegration::~KadasGpxIntegration()
{
  kApp->mainWindow()->removeCustomDropHandler( &mDropHandler );
}

KadasItemLayer *KadasGpxIntegration::getOrCreateLayer()
{
  return KadasItemLayerRegistry::getOrCreateItemLayer( KadasItemLayerRegistry::StandardLayer::RoutesLayer );
}

QgsAnnotationLayer *KadasGpxIntegration::getOrCreateAnnotationLayer()
{
  if ( !mLastAnnotationLayer )
  {
    mLastAnnotationLayer = KadasAnnotationLayerRegistry::getOrCreateAnnotationLayer( KadasAnnotationLayerRegistry::StandardLayer::RoutesLayer );
  }
  return mLastAnnotationLayer;
}

void KadasGpxIntegration::toggleAnnotation( bool active, Variant variant )
{
  QgsMapCanvas *canvas = kApp->mainWindow()->mapCanvas();
  QAction *action = qobject_cast<QAction *>( QObject::sender() );
  if ( !active )
  {
    if ( canvas->mapTool() && canvas->mapTool()->action() == action )
      canvas->unsetMapTool( canvas->mapTool() );
    return;
  }

  const QString typeId = ( variant == Variant::Waypoint ) ? KadasGpxWaypointAnnotationItem::itemTypeId() : KadasGpxRouteAnnotationItem::itemTypeId();

  KadasAnnotationItemController *controller = KadasAnnotationControllerRegistry::instance()->controllerFor( typeId );
  if ( !controller )
    return;

  QgsAnnotationLayer *layer = getOrCreateAnnotationLayer();
  if ( !layer )
    return;

  KadasMapToolCreateAnnotationItem *tool = new KadasMapToolCreateAnnotationItem( canvas, controller, layer );
  tool->setMultipart( false );
  tool->setAction( action );

  kApp->mainWindow()->layerTreeView()->setCurrentLayer( layer );
  kApp->mainWindow()->layerTreeView()->setLayerVisible( layer, true );
  canvas->setMapTool( tool );
}

void KadasGpxIntegration::openGpx()
{
  QString lastDir = QgsSettings().value( "/UI/lastImportExportDir", "." ).toString();
  QStringList filenames = QFileDialog::getOpenFileNames( kApp->mainWindow(), tr( "Import GPX" ), lastDir, tr( "GPX Files (*.gpx)" ) );
  if ( filenames.isEmpty() )
  {
    return;
  }
  QgsSettings().setValue( "/UI/lastImportExportDir", QFileInfo( filenames[0] ).absolutePath() );

  QStringList errors;
  for ( const QString &filename : filenames )
  {
    QString errorMsg;
    if ( !importGpx( filename, errorMsg ) )
    {
      errors.append( QString( "%1: %2" ).arg( QFileInfo( filename ).completeBaseName() ).arg( errorMsg ) );
    }
  }

  if ( errors.isEmpty() )
  {
    kApp->mainWindow()->messageBar()->pushMessage( tr( "GPX import completed" ), Qgis::Info, 5 );
  }
  else
  {
    QMessageBox::critical( kApp->mainWindow(), tr( "GPX import failed" ), tr( "The following files could not be imported:\n%1" ).arg( errors.join( "\n" ) ) );
  }
}

bool KadasGpxIntegration::importGpx( const QString &filename, QString &errorMsg )
{
  kApp->unsetMapTool();

  const QString layerName = QFileInfo( filename ).baseName();
  const QgsCoordinateReferenceSystem wgs84( QStringLiteral( "EPSG:4326" ) );
  QgsAnnotationLayer *layer = KadasAnnotationLayerHelpers::createLayer( layerName, wgs84 );
  QgsProject::instance()->addMapLayer( layer );

  QFile file( filename );
  if ( !file.open( QIODevice::ReadOnly ) )
  {
    errorMsg = tr( "Failed to open the input file." );
    return false;
  }

  QDomDocument doc;
  if ( !doc.setContent( &file ) )
  {
    errorMsg = tr( "Failed to read input file." );
    return false;
  }
  const QDomNodeList wpts = doc.elementsByTagName( "wpt" );
  for ( int i = 0, n = wpts.size(); i < n; ++i )
  {
    const QDomElement wptEl = wpts.at( i ).toElement();
    const double lat = wptEl.attribute( "lat" ).toDouble();
    const double lon = wptEl.attribute( "lon" ).toDouble();
    const QString name = wptEl.firstChildElement( "name" ).text();

    auto *waypoint = new KadasGpxWaypointAnnotationItem( QgsPoint( lon, lat ) );
    waypoint->setName( name );
    layer->addItem( waypoint );
  }
  const auto readRouteParts = []( const QDomElement &el, const QString &tagName ) {
    auto *line = new QgsLineString();
    const QDomNodeList pts = ( tagName == QStringLiteral( "trk" ) ) ? el.firstChildElement( "trkseg" ).elementsByTagName( "trkpt" ) : el.elementsByTagName( "rtept" );
    for ( int j = 0, m = pts.size(); j < m; ++j )
    {
      const QDomElement ptEl = pts.at( j ).toElement();
      const double lat = ptEl.attribute( "lat" ).toDouble();
      const double lon = ptEl.attribute( "lon" ).toDouble();
      line->addVertex( QgsPoint( lon, lat ) );
    }
    return line;
  };
  const QDomNodeList rtes = doc.elementsByTagName( "rte" );
  for ( int i = 0, n = rtes.size(); i < n; ++i )
  {
    const QDomElement rteEl = rtes.at( i ).toElement();
    const QString name = rteEl.firstChildElement( "name" ).text();
    const QString number = rteEl.firstChildElement( "number" ).text();
    auto *route = new KadasGpxRouteAnnotationItem( readRouteParts( rteEl, QStringLiteral( "rte" ) ) );
    route->setName( name );
    route->setNumber( number );
    layer->addItem( route );
  }
  const QDomNodeList trks = doc.elementsByTagName( "trk" );
  for ( int i = 0, n = trks.size(); i < n; ++i )
  {
    const QDomElement trkEl = trks.at( i ).toElement();
    const QString name = trkEl.firstChildElement( "name" ).text();
    const QString number = trkEl.firstChildElement( "number" ).text();
    auto *route = new KadasGpxRouteAnnotationItem( readRouteParts( trkEl, QStringLiteral( "trk" ) ) );
    route->setName( name );
    route->setNumber( number );
    layer->addItem( route );
  }
  return true;
}

void KadasGpxIntegration::saveGpx()
{
  kApp->unsetMapTool();

  QDialog dialog;
  dialog.setWindowTitle( tr( "Export to GPX" ) );
  dialog.setLayout( new QVBoxLayout );
  KadasLayerSelectionWidget *layerSelectionWidget = new KadasLayerSelectionWidget( kApp->mainWindow()->mapCanvas(), kApp->mainWindow()->layerTreeView(), []( QgsMapLayer *layer ) {
    if ( auto *itemLayer = dynamic_cast<KadasItemLayer *>( layer ) )
    {
      for ( KadasMapItem *item : itemLayer->items() )
      {
        if ( dynamic_cast<KadasGpxWaypointItem *>( item ) || dynamic_cast<KadasGpxRouteItem *>( item ) )
        {
          return true;
        }
      }
      return false;
    }
    if ( auto *annoLayer = dynamic_cast<QgsAnnotationLayer *>( layer ) )
    {
      const QMap<QString, QgsAnnotationItem *> items = annoLayer->items();
      for ( auto it = items.constBegin(); it != items.constEnd(); ++it )
      {
        if ( dynamic_cast<KadasGpxWaypointAnnotationItem *>( it.value() ) || dynamic_cast<KadasGpxRouteAnnotationItem *>( it.value() ) )
        {
          return true;
        }
      }
    }
    return false;
  } );
  layerSelectionWidget->setLabel( tr( "Select layer to export:" ) );
  dialog.layout()->addWidget( layerSelectionWidget );
  QDialogButtonBox *bbox = new QDialogButtonBox( QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal );
  connect( bbox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept );
  connect( bbox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject );
  dialog.layout()->addWidget( bbox );

  if ( dialog.exec() != QDialog::Accepted )
  {
    return;
  }
  QgsMapLayer *selectedLayer = layerSelectionWidget->getSelectedLayer();

  QString lastDir = QgsSettings().value( "/UI/lastImportExportDir", "." ).toString();
  QString filename = QFileDialog::getSaveFileName( kApp->mainWindow(), tr( "Export to GPX" ), QDir( lastDir ).absoluteFilePath( selectedLayer->name() + ".gpx" ), tr( "GPX Files (*.gpx)" ) );
  if ( filename.isEmpty() )
  {
    return;
  }
  QgsSettings().setValue( "/UI/lastImportExportDir", QFileInfo( filename ).absolutePath() );
  if ( !filename.endsWith( ".gpx", Qt::CaseInsensitive ) )
  {
    filename += ".gpx";
  }

  QFile file( filename );
  if ( !file.open( QIODevice::WriteOnly ) )
  {
    kApp->mainWindow()->messageBar()->pushMessage( tr( "GPX export failed" ), tr( "Cannot write to file" ), Qgis::Critical, 5 );
    return;
  }

  QDomDocument doc;
  QDomElement gpxEl = doc.createElement( "gpx" );
  gpxEl.setAttribute( "version", "1.1" );
  gpxEl.setAttribute( "creator", "kadas" );
  doc.appendChild( gpxEl );

  const QgsCoordinateTransform layerToWgs84( selectedLayer->crs(), QgsCoordinateReferenceSystem( "EPSG:4326" ), QgsProject::instance()->transformContext() );

  const auto appendWaypoint = [&]( const QgsPointXY &layerPos, const QString &name ) {
    const QgsPointXY pt = layerToWgs84.transform( layerPos );
    QDomElement wptEl = doc.createElement( "wpt" );
    wptEl.setAttribute( "lon", pt.x() );
    wptEl.setAttribute( "lat", pt.y() );
    QDomElement nameEl = doc.createElement( "name" );
    nameEl.appendChild( doc.createTextNode( name ) );
    wptEl.appendChild( nameEl );
    gpxEl.appendChild( wptEl );
  };
  const auto appendRoute = [&]( const QgsAbstractGeometry *geom, const QString &name, const QString &number ) {
    QDomElement rteEl = doc.createElement( "rte" );
    QDomElement nameEl = doc.createElement( "name" );
    nameEl.appendChild( doc.createTextNode( name ) );
    rteEl.appendChild( nameEl );
    QDomElement numberEl = doc.createElement( "number" );
    numberEl.appendChild( doc.createTextNode( number ) );
    rteEl.appendChild( numberEl );
    QgsPoint pt;
    QgsVertexId vidx;
    while ( geom->nextVertex( vidx, pt ) )
    {
      const QgsPointXY wgs = layerToWgs84.transform( pt );
      QDomElement rteptEl = doc.createElement( "rtept" );
      rteptEl.setAttribute( "lon", wgs.x() );
      rteptEl.setAttribute( "lat", wgs.y() );
      rteEl.appendChild( rteptEl );
    }
    gpxEl.appendChild( rteEl );
  };

  if ( auto *itemLayer = dynamic_cast<KadasItemLayer *>( selectedLayer ) )
  {
    for ( const KadasMapItem *item : itemLayer->items() )
    {
      if ( const auto *waypoint = dynamic_cast<const KadasGpxWaypointItem *>( item ) )
      {
        appendWaypoint( waypoint->point(), waypoint->name() );
      }
      else if ( const auto *route = dynamic_cast<const KadasGpxRouteItem *>( item ) )
      {
        appendRoute( route->geometry(), route->name(), route->number() );
      }
    }
  }
  else if ( auto *annoLayer = dynamic_cast<QgsAnnotationLayer *>( selectedLayer ) )
  {
    const QMap<QString, QgsAnnotationItem *> items = annoLayer->items();
    for ( auto it = items.constBegin(); it != items.constEnd(); ++it )
    {
      if ( const auto *waypoint = dynamic_cast<const KadasGpxWaypointAnnotationItem *>( it.value() ) )
      {
        appendWaypoint( waypoint->geometry(), waypoint->name() );
      }
      else if ( const auto *route = dynamic_cast<const KadasGpxRouteAnnotationItem *>( it.value() ) )
      {
        appendRoute( route->geometry(), route->name(), route->number() );
      }
    }
  }

  file.write( doc.toString().toLocal8Bit() );

  kApp->mainWindow()->messageBar()->pushMessage( tr( "GPX export completed" ), Qgis::Info, 5 );
}

bool KadasGpxDropHandler::canHandleMimeData( const QMimeData *data )
{
  for ( const QUrl &url : data->urls() )
  {
    if ( url.toLocalFile().endsWith( ".gpx", Qt::CaseInsensitive ) )
    {
      return true;
    }
  }
  return false;
}

bool KadasGpxDropHandler::handleMimeDataV2( const QMimeData *data )
{
  int handled = 0;
  for ( const QUrl &url : data->urls() )
  {
    QString path = url.toLocalFile();
    QStringList errors;
    if ( path.endsWith( ".gpx", Qt::CaseInsensitive ) )
    {
      ++handled;
      QString errMsg;
      if ( !KadasGpxIntegration::importGpx( path, errMsg ) )
      {
        errors.append( QString( "%1: %2" ).arg( QFileInfo( path ).fileName() ).arg( errMsg ) );
      }
    }
    if ( handled > 0 )
    {
      if ( errors.isEmpty() )
      {
        kApp->mainWindow()->messageBar()->pushMessage( tr( "GPX import completed" ), Qgis::Info, 5 );
      }
      else
      {
        QMessageBox::critical( kApp->mainWindow(), tr( "GPX import failed" ), tr( "The following files could not be imported:\n%1" ).arg( errors.join( "\n" ) ) );
      }
    }
  }
  return handled > 0;
}
