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

#include <qgis/qgscoordinatereferencesystem.h>
#include <qgis/qgslinestring.h>
#include <qgis/qgsmessagebar.h>
#include <qgis/qgsmultilinestring.h>
#include <qgis/qgsmultipoint.h>

#include <kadas/gui/kadasitemlayer.h>
#include <kadas/gui/kadaslayerselectionwidget.h>
#include <kadas/gui/mapitemeditors/kadasgpxrouteeditor.h>
#include <kadas/gui/mapitemeditors/kadasgpxwaypointeditor.h>
#include <kadas/gui/mapitems/kadasgpxrouteitem.h>
#include <kadas/gui/mapitems/kadasgpxwaypointitem.h>
#include <kadas/gui/mapitems/kadaslineitem.h>
#include <kadas/gui/maptools/kadasmaptoolcreateitem.h>

#include <kadas/app/kadasapplication.h>
#include <kadas/app/kadasgpxintegration.h>
#include <kadas/app/kadasmainwindow.h>

KadasGpxIntegration::KadasGpxIntegration( QAction *actionWaypoint, QAction *actionRoute, QAction *actionExportGpx, QAction *actionImportGpx, QObject *parent )
  : QObject( parent )
{

  KadasMapToolCreateItem::ItemFactory waypointFactory = [ = ]
  {
    return new KadasGpxWaypointItem();
  };
  KadasMapToolCreateItem::ItemFactory routeFactory = [ = ]
  {
    return new KadasGpxRouteItem();
  };

  connect( actionWaypoint, &QAction::triggered, this, [ = ]( bool active ) { toggleCreateItem( active, waypointFactory ); } );
  connect( actionRoute, &QAction::triggered, this, [ = ]( bool active ) { toggleCreateItem( active, routeFactory ); } );
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
  return KadasItemLayerRegistry::getOrCreateItemLayer( KadasItemLayerRegistry::RoutesLayer );
}

void KadasGpxIntegration::toggleCreateItem( bool active, const std::function<KadasMapItem*() > &itemFactory )
{
  QgsMapCanvas *canvas = kApp->mainWindow()->mapCanvas();
  QAction *action = qobject_cast<QAction *> ( QObject::sender() );
  if ( active )
  {
    KadasMapToolCreateItem *tool = new KadasMapToolCreateItem( canvas, itemFactory, getOrCreateLayer() );
    tool->setAction( action );
    KadasLayerSelectionWidget::LayerFilter filter = []( QgsMapLayer * layer ) { return dynamic_cast<KadasItemLayer *>( layer ); };
    KadasLayerSelectionWidget::LayerCreator creator = []( const QString & name ) { return KadasItemLayerRegistry::getOrCreateItemLayer( name ); };
    tool->showLayerSelection( true, kApp->mainWindow()->layerTreeView(), filter, creator );
    kApp->mainWindow()->layerTreeView()->setCurrentLayer( getOrCreateLayer() );
    kApp->mainWindow()->layerTreeView()->setLayerVisible( getOrCreateLayer(), true );
    canvas->setMapTool( tool );
  }
  else if ( !active && canvas->mapTool() && canvas->mapTool()->action() == action )
  {
    canvas->unsetMapTool( canvas->mapTool() );
  }
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
  KadasItemLayer *layer = KadasItemLayerRegistry::getOrCreateItemLayer( QFileInfo( filename ).baseName() );

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
  QDomNodeList wpts = doc.elementsByTagName( "wpt" );
  for ( int i = 0, n = wpts.size(); i < n; ++i )
  {
    QDomElement wptEl = wpts.at( i ).toElement();
    double lat = wptEl.attribute( "lat" ).toDouble();
    double lon = wptEl.attribute( "lon" ).toDouble();
    QString name = wptEl.firstChildElement( "name" ).text();

    KadasGpxWaypointItem *waypoint = new KadasGpxWaypointItem();
    waypoint->setName( name );
    waypoint->addPartFromGeometry( QgsPoint( lon, lat ) );
    layer->addItem( waypoint );
  }
  QDomNodeList rtes = doc.elementsByTagName( "rte" );
  for ( int i = 0, n = rtes.size(); i < n; ++i )
  {
    QgsLineString line;
    QDomElement rteEl = rtes.at( i ).toElement();
    QString name = rteEl.firstChildElement( "name" ).text();
    QString number = rteEl.firstChildElement( "name" ).text();
    QDomNodeList rtepts = rteEl.elementsByTagName( "rtept" );
    for ( int j = 0, m = rtepts.size(); j < m; ++j )
    {
      QDomElement rteptEl = rtepts.at( j ).toElement();
      double lat = rteptEl.attribute( "lat" ).toDouble();
      double lon = rteptEl.attribute( "lon" ).toDouble();
      line.addVertex( QgsPoint( lon, lat ) );
    }
    KadasGpxRouteItem *route = new KadasGpxRouteItem();
    route->setName( name );
    route->setNumber( number );
    route->addPartFromGeometry( line );
    layer->addItem( route );
  }
  QDomNodeList trks = doc.elementsByTagName( "trk" );
  for ( int i = 0, n = trks.size(); i < n; ++i )
  {
    QgsLineString line;
    QDomElement trkEl = trks.at( i ).toElement();
    QString name = trkEl.firstChildElement( "name" ).text();
    QString number = trkEl.firstChildElement( "name" ).text();
    QDomNodeList trkpts = trkEl.firstChildElement( "trkseg" ).elementsByTagName( "trkpt" );
    for ( int j = 0, m = trkpts.size(); j < m; ++j )
    {
      QDomElement trkptEl = trkpts.at( j ).toElement();
      double lat = trkptEl.attribute( "lat" ).toDouble();
      double lon = trkptEl.attribute( "lon" ).toDouble();
      line.addVertex( QgsPoint( lon, lat ) );
    }
    KadasGpxRouteItem *route = new KadasGpxRouteItem();
    route->setName( name );
    route->setNumber( number );
    route->addPartFromGeometry( line );
    layer->addItem( route );
  }
  return true;
}

void KadasGpxIntegration::saveGpx()
{
  QDialog dialog;
  dialog.setWindowTitle( tr( "Export to GPX" ) );
  dialog.setLayout( new QVBoxLayout );
  KadasLayerSelectionWidget *layerSelectionWidget = new KadasLayerSelectionWidget( kApp->mainWindow()->mapCanvas(), kApp->mainWindow()->layerTreeView(), []( QgsMapLayer * layer )
  {
    if ( !dynamic_cast<KadasItemLayer *>( layer ) )
    {
      return false;
    }
    for ( KadasMapItem *item : static_cast<KadasItemLayer *>( layer )->items() )
    {
      if ( dynamic_cast<KadasGpxWaypointItem *>( item ) || dynamic_cast<KadasGpxRouteItem *>( item ) )
      {
        return true;
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
  KadasItemLayer *layer = static_cast<KadasItemLayer *>( layerSelectionWidget->getSelectedLayer() );

  QString lastDir = QgsSettings().value( "/UI/lastImportExportDir", "." ).toString();
  QString filename = QFileDialog::getSaveFileName( kApp->mainWindow(), tr( "Export to GPX" ), QDir( lastDir ).absoluteFilePath( layer->name() + ".gpx" ), tr( "GPX Files (*.gpx)" ) );
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

  for ( const KadasMapItem *item : layer->items() )
  {
    if ( dynamic_cast<const KadasGpxWaypointItem *>( item ) )
    {
      const KadasGpxWaypointItem *waypoint = static_cast<const KadasGpxWaypointItem *>( item );
      QgsPoint pt = waypoint->geometry()->vertexAt( QgsVertexId( 0, 0, 0 ) );
      QDomElement wptEl = doc.createElement( "wpt" );
      wptEl.setAttribute( "lon", pt.x() );
      wptEl.setAttribute( "lat", pt.y() );
      QDomElement nameEl = doc.createElement( "name" );
      QDomText nameText = doc.createTextNode( waypoint->name() );
      nameEl.appendChild( nameText );
      wptEl.appendChild( nameEl );
      gpxEl.appendChild( wptEl );
    }
    if ( dynamic_cast<const KadasGpxRouteItem *>( item ) )
    {
      const KadasGpxRouteItem *route = static_cast<const KadasGpxRouteItem *>( item );
      QDomElement rteEl = doc.createElement( "rte" );
      QDomElement nameEl = doc.createElement( "name" );
      QDomText nameText = doc.createTextNode( route->name() );
      nameEl.appendChild( nameText );
      rteEl.appendChild( nameEl );
      QDomElement numberEl = doc.createElement( "number" );
      QDomText numberText = doc.createTextNode( route->number() );
      numberEl.appendChild( numberText );
      rteEl.appendChild( numberEl );
      QgsPoint pt;
      QgsVertexId vidx;
      while ( route->geometry()->nextVertex( vidx, pt ) )
      {
        QDomElement rteptEl = doc.createElement( "rtept" );
        rteptEl.setAttribute( "lon", pt.x() );
        rteptEl.setAttribute( "lat", pt.y() );
        rteEl.appendChild( rteptEl );
      }
      gpxEl.appendChild( rteEl );
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

