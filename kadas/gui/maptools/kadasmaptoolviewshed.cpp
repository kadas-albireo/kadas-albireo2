/***************************************************************************
    kadasmaptoolviewshed.cpp
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

#include <QApplication>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QGridLayout>
#include <QLabel>
#include <QMessageBox>
#include <QProgressDialog>

#include <qgis/qgsmapcanvas.h>
#include <qgis/qgsmultisurface.h>
#include <qgis/qgspolygon.h>
#include <qgis/qgsproject.h>
#include <qgis/qgsrasterlayer.h>
#include <qgis/qgsrastershader.h>
#include <qgis/qgssettings.h>
#include <qgis/qgssinglebandpseudocolorrenderer.h>

#include <kadas/core/kadascoordinateformat.h>
#include <kadas/analysis/kadasviewshedfilter.h>
#include <kadas/gui/mapitems/kadascircularsectoritem.h>
#include <kadas/gui/mapitems/kadassymbolitem.h>
#include <kadas/gui/maptools/kadasmaptoolviewshed.h>
#include <kadas/gui/kadasmapcanvasitemmanager.h>


KadasViewshedDialog::KadasViewshedDialog( double radius, QWidget *parent )
  : QDialog( parent )
{
  setWindowTitle( tr( "Viewshed setup" ) );

  QgsUnitTypes::DistanceUnit vertDisplayUnit = KadasCoordinateFormat::instance()->getHeightDisplayUnit();

  QGridLayout *heightDialogLayout = new QGridLayout();

  heightDialogLayout->addWidget( new QLabel( tr( "Observer height:" ) ), 0, 0, 1, 1 );
  mSpinBoxObserverHeight = new QDoubleSpinBox();
  mSpinBoxObserverHeight->setRange( 0, 8000 );
  mSpinBoxObserverHeight->setDecimals( 1 );
  mSpinBoxObserverHeight->setValue( 2. );
  mSpinBoxObserverHeight->setSuffix( vertDisplayUnit == QgsUnitTypes::DistanceFeet ? " ft" : " m" );
  heightDialogLayout->addWidget( mSpinBoxObserverHeight, 0, 1, 1, 1 );

  heightDialogLayout->addWidget( new QLabel( tr( "Target height:" ) ), 1, 0, 1, 1 );
  mSpinBoxTargetHeight = new QDoubleSpinBox();
  mSpinBoxTargetHeight->setRange( 0, 8000 );
  mSpinBoxTargetHeight->setDecimals( 1 );
  mSpinBoxTargetHeight->setValue( 2. );
  mSpinBoxTargetHeight->setSuffix( vertDisplayUnit == QgsUnitTypes::DistanceFeet ? " ft" : " m" );
  heightDialogLayout->addWidget( mSpinBoxTargetHeight, 1, 1, 1, 1 );

  heightDialogLayout->addWidget( new QLabel( tr( "Heights relative to:" ) ), 2, 0, 1, 1 );
  mComboHeightMode = new QComboBox();
  mComboHeightMode->addItem( tr( "Ground" ), static_cast<int>( HeightRelToGround ) );
  mComboHeightMode->addItem( tr( "Sea level" ), static_cast<int>( HeightRelToSeaLevel ) );
  heightDialogLayout->addWidget( mComboHeightMode, 2, 1, 1, 1 );

  heightDialogLayout->addWidget( new QLabel( tr( "Radius:" ) ), 3, 0, 1, 1 );
  QDoubleSpinBox *spinRadius = new QDoubleSpinBox();
  spinRadius->setRange( 1, 1000000000 );
  spinRadius->setDecimals( 0 );
  spinRadius->setValue( radius );
  spinRadius->setSuffix( " m" );
  spinRadius->setKeyboardTracking( false );
  connect( spinRadius, qOverload<double> ( &QDoubleSpinBox::valueChanged ), this, &KadasViewshedDialog::radiusChanged );
  heightDialogLayout->addWidget( spinRadius, 3, 1, 1, 1 );

  heightDialogLayout->addWidget( new QLabel( tr( "Display:" ) ), 4, 0, 1, 1 );
  mDisplayModeCombo = new QComboBox();
  mDisplayModeCombo->addItem( tr( "Visible area" ) );
  mDisplayModeCombo->addItem( tr( "Invisible area" ) );
  heightDialogLayout->addWidget( mDisplayModeCombo, 4, 1, 1, 1 );

  heightDialogLayout->addWidget( new QLabel( tr( "Accuracy:" ) ), 5, 0, 1, 1 );
  mAccuracySlider = new QSlider( Qt::Horizontal );
  mAccuracySlider->setRange( 1, 10 );
  mAccuracySlider->setTickPosition( QSlider::TicksBelow );
  mAccuracySlider->setTickInterval( 1 );
  heightDialogLayout->addWidget( mAccuracySlider, 5, 1, 1, 1 );

  QWidget *labelWidget = new QWidget( this );
  labelWidget->setLayout( new QHBoxLayout );
  labelWidget->layout()->setContentsMargins( 0, 0, 0, 0 );
  labelWidget->layout()->addWidget( new QLabel( QString( "<small>%1</small>" ).arg( tr( "Accurate" ) ) ) );
  labelWidget->layout()->addItem( new QSpacerItem( 1, 1, QSizePolicy::Expanding ) );
  labelWidget->layout()->addWidget( new QLabel( QString( "<small>%1</small>" ).arg( tr( "Fast" ) ) ) );
  heightDialogLayout->addWidget( labelWidget, 6, 1, 1, 1 );

  QDialogButtonBox *bbox = new QDialogButtonBox( QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal );
  connect( bbox, &QDialogButtonBox::accepted, this, &QDialog::accept );
  connect( bbox, &QDialogButtonBox::rejected, this, &QDialog::reject );
  heightDialogLayout->addWidget( bbox, 7, 0, 1, 2 );

  setLayout( heightDialogLayout );
  setFixedSize( sizeHint() );
}

double KadasViewshedDialog::getObserverHeight() const
{
  return mSpinBoxObserverHeight->value();
}

double KadasViewshedDialog::getTargetHeight() const
{
  return mSpinBoxTargetHeight->value();
}

bool KadasViewshedDialog::getHeightRelativeToGround() const
{
  return static_cast<HeightMode>( mComboHeightMode->itemData( mComboHeightMode->currentIndex() ).toInt() ) == HeightRelToGround;
}

KadasViewshedDialog::DisplayMode KadasViewshedDialog::getDisplayMode() const
{
  return static_cast<DisplayMode>( mDisplayModeCombo->currentIndex() );
}

int KadasViewshedDialog::getAccuracyFactor() const
{
  return mAccuracySlider->value();
}

///////////////////////////////////////////////////////////////////////////////

KadasMapToolViewshed::KadasMapToolViewshed( QgsMapCanvas *mapCanvas )
  : KadasMapToolCreateItem( mapCanvas, itemFactory( mapCanvas ) )
{
  setCursor( Qt::ArrowCursor );
  connect( this, &KadasMapToolCreateItem::partFinished, this, &KadasMapToolViewshed::drawFinished );
}

KadasMapToolCreateItem::ItemFactory KadasMapToolViewshed::itemFactory( const QgsMapCanvas *canvas ) const
{
  return [ = ]
  {
    KadasCircularSectorItem *item = new KadasCircularSectorItem( canvas->mapSettings().destinationCrs() );
    return item;
  };
}

void KadasMapToolViewshed::drawFinished()
{
  QString layerid = QgsProject::instance()->readEntry( "Heightmap", "layer" );
  QgsMapLayer *layer = QgsProject::instance()->mapLayer( layerid );
  if ( !layer || layer->type() != QgsMapLayerType::RasterLayer )
  {
    emit messageEmitted( tr( "No heightmap is defined in the project." ), Qgis::Warning );
    clear();
    return;
  }

  const KadasCircularSectorItem *item = dynamic_cast<const KadasCircularSectorItem *>( currentItem() );
  if ( !item )
  {
    clear();
    return;
  }

  QgsPointXY center = item->constState()->centers.last();
  double curRadius = item->constState()->radii.last();

  QgsCoordinateReferenceSystem canvasCrs = canvas()->mapSettings().destinationCrs();
  QgsUnitTypes::DistanceUnit measureUnit = canvasCrs.mapUnits();
  curRadius *= QgsUnitTypes::fromUnitToUnitFactor( canvasCrs.mapUnits(), QgsUnitTypes::DistanceMeters );

  KadasViewshedDialog viewshedDialog( curRadius );
  connect( &viewshedDialog, &KadasViewshedDialog::radiusChanged, this, &KadasMapToolViewshed::adjustRadius );
  if ( viewshedDialog.exec() == QDialog::Rejected )
  {
    clear();
    return;
  }

  QString outputFileName = QString( "viewshed_%1,%2.tif" ).arg( center.x() ).arg( center.y() );
  QString outputFile = QgsProject::instance()->createAttachedFile( outputFileName );

  QVector<QgsPointXY> filterRegion;
  QgsPolygonXY poly = QgsGeometry( item->geometry()->geometryN( 0 )->clone() ).asPolygon();
  if ( !poly.isEmpty() )
  {
    filterRegion = poly.front();
  }
  center = item->constState()->centers.last();
  curRadius = item->constState()->radii.last();

  if ( mCanvas->mapSettings().mapUnits() == QgsUnitTypes::DistanceDegrees )
  {
    // Need to compute radius in meters
    QgsDistanceArea da;
    da.setEllipsoid( QgsProject::instance()->readEntry( "Measure", "/Ellipsoid", GEO_NONE ) );
    curRadius = da.measureLine( center, QgsPoint( center.x() + curRadius, center.y() ) );
    curRadius = da.convertLengthMeasurement( curRadius, QgsUnitTypes::DistanceMeters );
  }

  double heightConv = QgsUnitTypes::fromUnitToUnitFactor( KadasCoordinateFormat::instance()->getHeightDisplayUnit(), QgsUnitTypes::DistanceMeters );

  QProgressDialog p( tr( "Calculating viewshed..." ), tr( "Abort" ), 0, 0 );
  p.setWindowTitle( tr( "Viewshed" ) );
  p.setWindowModality( Qt::ApplicationModal );
  bool displayVisible = viewshedDialog.getDisplayMode() == KadasViewshedDialog::DisplayVisibleArea;
  int accuracyFactor = viewshedDialog.getAccuracyFactor();
  QApplication::setOverrideCursor( Qt::WaitCursor );
  bool success = KadasViewshedFilter::computeViewshed( layer->source(), outputFile, "GTiff", center, canvasCrs, viewshedDialog.getObserverHeight() * heightConv, viewshedDialog.getTargetHeight() * heightConv, viewshedDialog.getHeightRelativeToGround(), curRadius, QgsUnitTypes::DistanceMeters, filterRegion, displayVisible, accuracyFactor, &p );
  QApplication::restoreOverrideCursor();
  if ( success )
  {
    QgsRasterLayer *layer = new QgsRasterLayer( outputFile, tr( "Viewshed [%1]" ).arg( center.toString() ) );
    QgsColorRampShader *rampShader = new QgsColorRampShader();
    if ( displayVisible )
    {
      QList<QgsColorRampShader::ColorRampItem> colorRampItems = QList<QgsColorRampShader::ColorRampItem>()
          << QgsColorRampShader::ColorRampItem( 255, QColor( 0, 255, 0 ), tr( "Visible" ) );
      rampShader->setColorRampItemList( colorRampItems );
    }
    else
    {
      QList<QgsColorRampShader::ColorRampItem> colorRampItems = QList<QgsColorRampShader::ColorRampItem>()
          << QgsColorRampShader::ColorRampItem( 0, QColor( 255, 0, 0 ), tr( "Invisible" ) );
      rampShader->setColorRampItemList( colorRampItems );
    }
    QgsRasterShader *shader = new QgsRasterShader();
    shader->setRasterShaderFunction( rampShader );
    QgsSingleBandPseudoColorRenderer *renderer = new QgsSingleBandPseudoColorRenderer( 0, 1, shader );
    layer->setRenderer( renderer );
    QgsProject::instance()->addMapLayer( layer );

    KadasPinItem *pin = new KadasPinItem( canvasCrs, this );
    pin->associateToLayer( layer );
    pin->setPosition( KadasItemPos::fromPoint( center ) );
    KadasMapCanvasItemManager::addItem( pin );
  }
  else
  {
    QMessageBox::critical( 0, tr( "Error" ), tr( "Failed to compute viewshed." ) );
  }
  clear();
}

void KadasMapToolViewshed::adjustRadius( double newRadius )
{
  QgsUnitTypes::DistanceUnit measureUnit = QgsUnitTypes::DistanceMeters;
  QgsUnitTypes::DistanceUnit targetUnit = canvas()->mapSettings().destinationCrs().mapUnits();
  newRadius *= QgsUnitTypes::fromUnitToUnitFactor( measureUnit, targetUnit );

  KadasCircularSectorItem *item = dynamic_cast<KadasCircularSectorItem *>( mutableItem() );
  if ( !item )
  {
    return;
  }

  KadasCircularSectorItem::State *state = item->constState()->clone();
  state->radii.last() = newRadius;
  item->setState( state );
  delete state;
}
