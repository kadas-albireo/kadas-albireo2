/***************************************************************************
    kadasmaptoolguidegrid.cpp
    -------------------------
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
#include <QPushButton>

#include <qgis/qgslayertreeview.h>
#include <qgis/qgsmapcanvas.h>
#include <qgis/qgsmapmouseevent.h>
#include <qgis/qgsproject.h>

#include <kadas/gui/kadaslayerselectionwidget.h>
#include <kadas/app/guidegrid/kadasguidegridlayer.h>
#include <kadas/app/guidegrid/kadasmaptoolguidegrid.h>

KadasMapToolGuideGrid::KadasMapToolGuideGrid( QgsMapCanvas *canvas, QgsLayerTreeView *layerTreeView, QgsMapLayer *layer )
  : QgsMapTool( canvas )
{
  if ( !layer )
  {
    for ( QgsMapLayer *projLayer : QgsProject::instance()->mapLayers() )
    {
      if ( dynamic_cast<KadasGuideGridLayer *>( projLayer ) )
      {
        layer = projLayer;
        break;
      }
    }
  }

  mWidget = new KadasGuideGridWidget( canvas, layerTreeView );
  if ( layer )
  {
    mWidget->setLayer( layer );
  }
  setCursor( Qt::ArrowCursor );
  connect( mWidget, &KadasGuideGridWidget::requestPick, this, &KadasMapToolGuideGrid::setPickMode );
  connect( mWidget, &KadasGuideGridWidget::close, this, &KadasMapToolGuideGrid::close );

  mWidget->show();
}

KadasMapToolGuideGrid::~KadasMapToolGuideGrid()
{
  delete mWidget;
}

void KadasMapToolGuideGrid::setPickMode( PickMode pickMode )
{
  setCursor( QCursor( pickMode == PICK_NONE ? Qt::ArrowCursor : Qt::CrossCursor ) );
  mPickMode = pickMode;
}

void KadasMapToolGuideGrid::close()
{
  canvas()->unsetMapTool( this );
}

void KadasMapToolGuideGrid::canvasReleaseEvent( QgsMapMouseEvent *e )
{
  if ( mPickMode != PICK_NONE )
  {
    mWidget->pointPicked( mPickMode, e->mapPoint() );
    mPickMode = PICK_NONE;
    setCursor( Qt::ArrowCursor );
  }
  else if ( e->button() == Qt::RightButton )
  {
    canvas()->unsetMapTool( this );
  }
}

void KadasMapToolGuideGrid::keyReleaseEvent( QKeyEvent *e )
{
  if ( e->key() == Qt::Key_Escape )
  {
    if ( mPickMode != PICK_NONE )
    {
      mPickMode = PICK_NONE;
      setCursor( Qt::ArrowCursor );
    }
    else
    {
      canvas()->unsetMapTool( this );
    }
  }
}


static QRegExp g_cooRegExp( "^\\s*(\\d+\\.?\\d*)[,\\s]?\\s*(\\d+\\.?\\d*)\\s*$" );

KadasGuideGridWidget::KadasGuideGridWidget( QgsMapCanvas *canvas, QgsLayerTreeView *layerTreeView )
  : KadasBottomBar( canvas )
{
  setLayout( new QHBoxLayout );
  layout()->setSpacing( 10 );

  QWidget *base = new QWidget();
  ui.setupUi( base );
  layout()->addWidget( base );

  QPushButton *closeButton = new QPushButton();
  closeButton->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Preferred );
  closeButton->setIcon( QIcon( ":/kadas/icons/close" ) );
  closeButton->setToolTip( tr( "Close" ) );
  connect( closeButton, &QPushButton::clicked, this, &KadasGuideGridWidget::close );
  layout()->addWidget( closeButton );
  layout()->setAlignment( closeButton, Qt::AlignTop );

  auto layerFilter = []( QgsMapLayer * layer ) { return dynamic_cast<KadasGuideGridLayer *>( layer ) != nullptr; };
  auto layerCreator = [this]( const QString & name ) { return createLayer( name ); };
  mLayerSelectionWidget = new KadasLayerSelectionWidget( canvas, layerTreeView, layerFilter, layerCreator );
  mLayerSelectionWidget->createLayerIfEmpty( tr( "Guidegrid" ) );
  ui.layerSelectionWidgetHolder->addWidget( mLayerSelectionWidget );

  connect( ui.lineEditTopLeft, &QLineEdit::editingFinished, this, &KadasGuideGridWidget::topLeftEdited );
  connect( ui.toolButtonPickTopLeft, &QToolButton::clicked, this, &KadasGuideGridWidget::pickTopLeftPos );

  connect( ui.lineEditBottomRight, &QLineEdit::editingFinished, this, &KadasGuideGridWidget::bottomRightEdited );
  connect( ui.toolButtonPickBottomRight, &QToolButton::clicked, this, &KadasGuideGridWidget::pickBottomRight );

  ui.spinBoxCols->setRange( 1, 10000 );
  connect( ui.spinBoxCols, qOverload<int>( &QSpinBox::valueChanged ), this, &KadasGuideGridWidget::updateIntervals );

  ui.spinBoxRows->setRange( 1, 10000 );
  connect( ui.spinBoxRows, qOverload<int>( &QSpinBox::valueChanged ), this, &KadasGuideGridWidget::updateIntervals );

  ui.spinBoxWidth->setRange( 1, 99999999 );
  connect( ui.spinBoxWidth, qOverload<double>( &QDoubleSpinBox::valueChanged ), this, &KadasGuideGridWidget::updateBottomRight );

  ui.spinBoxHeight->setRange( 1, 99999999 );
  connect( ui.spinBoxHeight, qOverload<double>( &QDoubleSpinBox::valueChanged ), this, &KadasGuideGridWidget::updateBottomRight );

  connect( ui.toolButtonLockHeight, &QToolButton::toggled, this, &KadasGuideGridWidget::updateLockIcon );
  connect( ui.toolButtonLockWidth, &QToolButton::toggled, this, &KadasGuideGridWidget::updateLockIcon );

  connect( ui.toolButtonColor, &QgsColorButton::colorChanged, this, &KadasGuideGridWidget::updateColor );
  connect( ui.spinBoxFontSize, qOverload<int>( &QSpinBox::valueChanged ), this, &KadasGuideGridWidget::updateFontSize );
  connect( ui.comboBoxLabeling, qOverload<int>( &QComboBox::currentIndexChanged ), this, &KadasGuideGridWidget::updateLabeling );

  connect( mLayerSelectionWidget, &KadasLayerSelectionWidget::selectedLayerChanged, this, &KadasGuideGridWidget::currentLayerChanged );
}

QgsMapLayer *KadasGuideGridWidget::createLayer( QString layerName )
{
  KadasGuideGridLayer *guideGridLayer = new KadasGuideGridLayer( layerName );
  guideGridLayer->setup( mCanvas->extent(), 10, 10, mCanvas->mapSettings().destinationCrs(), false, false );
  setLayer( guideGridLayer );
  return guideGridLayer;
}

void KadasGuideGridWidget::setLayer( QgsMapLayer *layer )
{
  if ( layer == mCurrentLayer )
  {
    return;
  }
  emit requestPick( KadasMapToolGuideGrid::PICK_NONE );
  mCurrentLayer = dynamic_cast<KadasGuideGridLayer *>( layer );
  if ( !mCurrentLayer )
  {
    ui.widgetLayerSetup->setEnabled( false );
    return;
  }
  mLayerSelectionWidget->blockSignals( true );
  mLayerSelectionWidget->setSelectedLayer( layer );
  mLayerSelectionWidget->blockSignals( false );

  mCrs = mCurrentLayer->crs();
  int prec = mCrs.mapUnits() == QgsUnitTypes::DistanceDegrees ? 3 : 0;
  mCurRect = mCurrentLayer->extent();
  ui.lineEditTopLeft->setText( QString( "%1, %2" ).arg( mCurRect.xMinimum(), 0, 'f', prec ).arg( mCurRect.yMaximum(), 0, 'f', prec ) );
  ui.lineEditBottomRight->setText( QString( "%1, %2" ).arg( mCurRect.xMaximum(), 0, 'f', prec ).arg( mCurRect.yMinimum(), 0, 'f', prec ) );
  ui.spinBoxCols->blockSignals( true );
  ui.spinBoxCols->setValue( mCurrentLayer->cols() );
  ui.spinBoxCols->blockSignals( false );
  ui.spinBoxWidth->blockSignals( true );
  ui.spinBoxWidth->setValue( mCurRect.width() / mCurrentLayer->cols() );
  ui.spinBoxWidth->blockSignals( false );
  ui.spinBoxRows->blockSignals( true );
  ui.spinBoxRows->setValue( mCurrentLayer->rows() );
  ui.spinBoxRows->blockSignals( false );
  ui.spinBoxHeight->blockSignals( true );
  ui.spinBoxHeight->setValue( mCurRect.height() / mCurrentLayer->rows() );
  ui.spinBoxHeight->blockSignals( false );
  ui.toolButtonLockHeight->blockSignals( true );
  ui.toolButtonLockHeight->setChecked( mCurrentLayer->rowSizeLocked() );
  ui.toolButtonLockHeight->setIcon( QIcon( ui.toolButtonLockHeight->isChecked() ? ":/images/themes/default/locked.svg" : ":/images/themes/default/unlocked.svg" ) );
  ui.toolButtonLockHeight->blockSignals( false );
  ui.toolButtonLockWidth->blockSignals( true );
  ui.toolButtonLockWidth->setChecked( mCurrentLayer->colSizeLocked() );
  ui.toolButtonLockWidth->setIcon( QIcon( ui.toolButtonLockWidth->isChecked() ? ":/images/themes/default/locked.svg" : ":/images/themes/default/unlocked.svg" ) );
  ui.toolButtonLockWidth->blockSignals( false );
  ui.toolButtonColor->setColor( mCurrentLayer->color() );
  ui.spinBoxFontSize->setValue( mCurrentLayer->fontSize() );
  ui.comboBoxLabeling->setCurrentIndex( mCurrentLayer->labelingMode() );
  updateIntervals();
  ui.widgetLayerSetup->setEnabled( true );
}

void KadasGuideGridWidget::pointPicked( KadasMapToolGuideGrid::PickMode pickMode, const QgsPointXY &pos )
{
  int prec = mCrs.mapUnits() == QgsUnitTypes::DistanceDegrees ? 3 : 0;
  if ( pickMode == KadasMapToolGuideGrid::PICK_TOP_LEFT )
  {
    ui.lineEditTopLeft->setText( QString( "%1, %2" ).arg( pos.x(), 0, 'f', prec ).arg( pos.y(), 0, 'f', prec ) );
    topLeftEdited();
  }
  else if ( pickMode == KadasMapToolGuideGrid::PICK_BOTTOM_RIGHT )
  {
    ui.lineEditBottomRight->setText( QString( "%1, %2" ).arg( pos.x(), 0, 'f', prec ).arg( pos.y(), 0, 'f', prec ) );
    bottomRightEdited();
  }
}

void KadasGuideGridWidget::topLeftEdited()
{
  QString text = ui.lineEditTopLeft->text();
  if ( g_cooRegExp.indexIn( text ) != -1 )
  {
    mCurRect.setXMinimum( g_cooRegExp.cap( 1 ).toDouble() );
    mCurRect.setYMaximum( g_cooRegExp.cap( 2 ).toDouble() );
    if ( ui.toolButtonLockWidth->isChecked() )
    {
      mCurRect.setXMaximum( mCurRect.xMinimum() + ui.spinBoxCols->value() * ui.spinBoxWidth->value() );
    }
    else
    {
      ui.spinBoxWidth->blockSignals( true );
      ui.spinBoxWidth->setValue( mCurRect.width() / ui.spinBoxCols->value() );
      ui.spinBoxWidth->blockSignals( false );
    }
    if ( ui.toolButtonLockHeight->isChecked() )
    {
      mCurRect.setYMinimum( mCurRect.yMaximum() - ui.spinBoxRows->value() * ui.spinBoxHeight->value() );
    }
    else
    {
      ui.spinBoxHeight->blockSignals( true );
      ui.spinBoxHeight->setValue( mCurRect.height() / ui.spinBoxRows->value() );
      ui.spinBoxHeight->blockSignals( false );
    }
    updateGrid();
  }
  else
  {
    int prec = mCrs.mapUnits() == QgsUnitTypes::DistanceDegrees ? 3 : 0;
    ui.lineEditTopLeft->setText( QString( "%1, %2" ).arg( mCurRect.xMinimum(), 0, 'f', prec ).arg( mCurRect.yMaximum(), 0, 'f', prec ) );
  }
}

void KadasGuideGridWidget::bottomRightEdited()
{
  QString text = ui.lineEditBottomRight->text();
  if ( g_cooRegExp.indexIn( text ) != -1 )
  {
    mCurRect.setXMaximum( g_cooRegExp.cap( 1 ).toDouble() );
    mCurRect.setYMinimum( g_cooRegExp.cap( 2 ).toDouble() );
    if ( ui.toolButtonLockWidth->isChecked() )
    {
      mCurRect.setXMinimum( mCurRect.xMaximum() - ui.spinBoxCols->value() * ui.spinBoxWidth->value() );
    }
    else
    {
      ui.spinBoxWidth->blockSignals( true );
      ui.spinBoxWidth->setValue( mCurRect.width() / ui.spinBoxCols->value() );
      ui.spinBoxWidth->blockSignals( false );
    }
    if ( ui.toolButtonLockHeight->isChecked() )
    {
      mCurRect.setYMaximum( mCurRect.yMinimum() + ui.spinBoxRows->value() * ui.spinBoxHeight->value() );
    }
    else
    {
      ui.spinBoxHeight->blockSignals( true );
      ui.spinBoxHeight->setValue( mCurRect.height() / ui.spinBoxRows->value() );
      ui.spinBoxHeight->blockSignals( false );
    }
    updateGrid();
  }
  else
  {
    int prec = mCrs.mapUnits() == QgsUnitTypes::DistanceDegrees ? 3 : 0;
    ui.lineEditBottomRight->setText( QString( "%1, %2" ).arg( mCurRect.xMaximum(), 0, 'f', prec ).arg( mCurRect.yMinimum(), 0, 'f', prec ) );
  }
}

void KadasGuideGridWidget::updateIntervals()
{
  int cols = ui.spinBoxCols->value();
  int rows = ui.spinBoxRows->value();
  QgsRectangle newRect = mCurRect;
  if ( ui.toolButtonLockWidth->isChecked() )
  {
    newRect.setXMaximum( newRect.xMinimum() + ui.spinBoxWidth->value() * cols );
  }
  else
  {
    ui.spinBoxWidth->blockSignals( true );
    ui.spinBoxWidth->setValue( cols > 0 ? std::abs( mCurRect.width() ) / cols : 0. );
    ui.spinBoxWidth->blockSignals( false );
  }
  if ( ui.toolButtonLockHeight->isChecked() )
  {
    newRect.setYMinimum( newRect.yMaximum() - ui.spinBoxHeight->value() * rows );
  }
  else
  {
    ui.spinBoxHeight->blockSignals( true );
    ui.spinBoxHeight->setValue( rows > 0 ? std::abs( mCurRect.height() ) / rows : 0. );
    ui.spinBoxHeight->blockSignals( false );
  }
  mCurRect = newRect;
  updateGrid();
}

void KadasGuideGridWidget::updateBottomRight()
{
  int prec = mCrs.mapUnits() == QgsUnitTypes::DistanceDegrees ? 3 : 0;
  mCurRect.setXMaximum( mCurRect.xMinimum() + ui.spinBoxCols->value() * ui.spinBoxWidth->value() );
  mCurRect.setYMinimum( mCurRect.yMaximum() - ui.spinBoxRows->value() * ui.spinBoxHeight->value() );
  ui.lineEditBottomRight->setText( QString( "%1, %2" ).arg( mCurRect.xMaximum(), 0, 'f', prec ).arg( mCurRect.yMinimum(), 0, 'f', prec ) );
  updateGrid();
}

void KadasGuideGridWidget::updateLockIcon( bool locked )
{
  QToolButton *button = qobject_cast<QToolButton *>( QObject::sender() );
  button->setIcon( QIcon( locked ? ":/kadas/icons/locked" : ":/kadas/icons/unlocked" ) );
  updateGrid();
}

void KadasGuideGridWidget::updateGrid()
{
  if ( !mCurrentLayer )
  {
    return;
  }
  int prec = mCrs.mapUnits() == QgsUnitTypes::DistanceDegrees ? 3 : 0;
  mCurRect.normalize();
  ui.lineEditTopLeft->setText( QString( "%1, %2" ).arg( mCurRect.xMinimum(), 0, 'f', prec ).arg( mCurRect.yMaximum(), 0, 'f', prec ) );
  ui.lineEditBottomRight->setText( QString( "%1, %2" ).arg( mCurRect.xMaximum(), 0, 'f', prec ).arg( mCurRect.yMinimum(), 0, 'f', prec ) );
  mCurrentLayer->setup( mCurRect, ui.spinBoxCols->value(), ui.spinBoxRows->value(), mCrs, ui.toolButtonLockWidth->isChecked(), ui.toolButtonLockHeight->isChecked() );
  mCurrentLayer->triggerRepaint();
}

void KadasGuideGridWidget::updateColor( const QColor &color )
{
  if ( !mCurrentLayer )
  {
    return;
  }
  mCurrentLayer->setColor( color );
  mCurrentLayer->triggerRepaint();
}

void KadasGuideGridWidget::updateFontSize( int fontSize )
{
  if ( !mCurrentLayer )
  {
    return;
  }
  mCurrentLayer->setFontSize( fontSize );
  mCurrentLayer->triggerRepaint();
}

void KadasGuideGridWidget::updateLabeling( int labelingMode )
{
  if ( !mCurrentLayer )
  {
    return;
  }
  mCurrentLayer->setLabelingMode( static_cast<KadasGuideGridLayer::LabellingMode>( labelingMode ) );
  mCurrentLayer->triggerRepaint();
}

void KadasGuideGridWidget::currentLayerChanged( QgsMapLayer *layer )
{
  KadasGuideGridLayer *guidegridLayer = dynamic_cast<KadasGuideGridLayer *>( layer );
  if ( guidegridLayer )
  {
    setLayer( guidegridLayer );
  }
  else
  {
    ui.widgetLayerSetup->setEnabled( false );
  }
}
