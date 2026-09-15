/***************************************************************************
    kadaslayerselectionwidget.cpp
    -----------------------------
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

#include <QComboBox>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QRegularExpression>
#include <QToolButton>

#include <qgis/qgsapplication.h>
#include <qgis/qgsiconutils.h>
#include <qgis/qgslayertree.h>
#include <qgis/qgslayertreeview.h>
#include <qgis/qgsmapcanvas.h>
#include <qgis/qgsproject.h>

#include "kadas/gui/kadaslayerselectionwidget.h"


KadasLayerSelectionWidget::KadasLayerSelectionWidget( QgsMapCanvas *canvas, QgsLayerTreeView *layerTreeView, LayerFilter filter, LayerCreator creator, QWidget *parent )
  : QWidget( parent )
  , mCanvas( canvas )
  , mLayerTreeView( layerTreeView )
  , mFilter( filter )
  , mCreator( creator )
{
  setLayout( new QHBoxLayout() );
  layout()->setSpacing( 2 );
  layout()->setContentsMargins( 0, 0, 0, 0 );

  mLabel = new QLabel( tr( "Layer:" ) );
  layout()->addWidget( mLabel );

  mLayersCombo = new QComboBox( this );
  // The widget is hosted both in horizontal tool bars and in the narrow side
  // panel, so let the combo use the room it is given and elide long layer
  // names rather than pinning it to a width that fits neither.
  mLayersCombo->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
  mLayersCombo->setMinimumContentsLength( 10 );
  mLayersCombo->setSizeAdjustPolicy( QComboBox::AdjustToMinimumContentsLengthWithIcon );
  connect( mLayersCombo, qOverload<int>( &QComboBox::currentIndexChanged ), this, qOverload<int>( &KadasLayerSelectionWidget::layerSelectionChanged ) );
  layout()->addWidget( mLayersCombo );

  if ( creator )
  {
    mNewLayerButton = new QToolButton();
    mNewLayerButton->setIcon( QgsApplication::getThemeIcon( "/mActionAdd.svg" ) );
    mNewLayerButton->setToolTip( tr( "Add a new layer" ) );
    connect( mNewLayerButton, &QToolButton::clicked, this, &KadasLayerSelectionWidget::createLayer );
    layout()->addWidget( mNewLayerButton );
  }

  connect( QgsProject::instance(), &QgsProject::layersAdded, this, &KadasLayerSelectionWidget::repopulateLayers );
  connect( QgsProject::instance(), &QgsProject::layersRemoved, this, &KadasLayerSelectionWidget::repopulateLayers );
  connect( mCanvas, &QgsMapCanvas::currentLayerChanged, this, &KadasLayerSelectionWidget::canvasCurrentLayerChanged );

  // Merely showing the widget must not make a layer current - that is for an
  // actual choice. Callers that want their layer activated say so by calling
  // setSelectedLayer() once the widget exists.
  mInitialPopulation = true;
  repopulateLayers();
  mInitialPopulation = false;
}

void KadasLayerSelectionWidget::createLayerIfEmpty( const QString &layerName )
{
  mNewLayerName = layerName;
  if ( mLayersCombo->count() == 0 && mCreator )
  {
    QgsMapLayer *layer = mCreator( layerName );
    if ( !layer )
      return;
    QgsProject::instance()->addMapLayer( layer );
    setSelectedLayer( layer );
  }
}

void KadasLayerSelectionWidget::setLabel( const QString &label )
{
  mLabel->setText( label );
  // An empty label means the host renders its own (a form row, for instance),
  // so don't reserve any space for it.
  mLabel->setVisible( !label.isEmpty() );
}

void KadasLayerSelectionWidget::setNewLayerName( const QString &name )
{
  mNewLayerName = name;
}

void KadasLayerSelectionWidget::setReadOnly( bool readOnly )
{
  mReadOnly = readOnly;
  mLayersCombo->setEnabled( !readOnly );
  if ( mNewLayerButton )
  {
    // A button that can never fire is just noise next to a read-only value.
    mNewLayerButton->setVisible( !readOnly );
  }
}

QgsMapLayer *KadasLayerSelectionWidget::getSelectedLayer() const
{
  return QgsProject::instance()->mapLayer( selectedLayerId() );
}

QString KadasLayerSelectionWidget::selectedLayerId() const
{
  const int idx = mLayersCombo->currentIndex();
  return idx >= 0 ? mLayersCombo->itemData( idx ).toString() : QString();
}

void KadasLayerSelectionWidget::repopulateLayers()
{
  // Avoid update while updating
  if ( mRepopulating )
  {
    return;
  }
  mRepopulating = true;

  const QString previousId = selectedLayerId();

  mLayersCombo->blockSignals( true );
  mLayersCombo->clear();
  const QList<QgsMapLayer *> layers = QgsProject::instance()->mapLayers().values();
  for ( QgsMapLayer *layer : layers )
  {
    if ( !mFilter || mFilter( layer ) )
    {
      connect( layer, &QgsMapLayer::nameChanged, this, &KadasLayerSelectionWidget::repopulateLayers, Qt::UniqueConnection );
      mLayersCombo->addItem( QgsIconUtils::iconForLayer( layer ), layer->name(), layer->id() );
    }
  }

  // What a read-only widget displays is a property of something that exists, so
  // keep it listed even where the filter would not offer it for a new item, and
  // never substitute another layer for it.
  if ( mReadOnly && !previousId.isEmpty() && mLayersCombo->findData( previousId ) < 0 )
  {
    if ( QgsMapLayer *shown = QgsProject::instance()->mapLayer( previousId ) )
      mLayersCombo->addItem( QgsIconUtils::iconForLayer( shown ), shown->name(), shown->id() );
  }

  // Keep pointing at the layer the user picked: adding or removing an unrelated
  // layer must not silently retarget a tool that is mid-drawing. Only once that
  // layer is gone fall back to the current layer, then to the first entry.
  int idx = previousId.isEmpty() ? -1 : mLayersCombo->findData( previousId );
  if ( !mReadOnly )
  {
    if ( idx < 0 && mCanvas && mCanvas->currentLayer() )
      idx = mLayersCombo->findData( mCanvas->currentLayer()->id() );
    if ( idx < 0 && mLayersCombo->count() > 0 )
      idx = 0;
  }
  mLayersCombo->setCurrentIndex( idx );
  mLayersCombo->setToolTip( idx >= 0 ? mLayersCombo->itemText( idx ) : QString() );
  mLayersCombo->blockSignals( false );

  mRepopulating = false;

  if ( selectedLayerId() != previousId )
  {
    layerSelectionChanged( idx );
  }
}

void KadasLayerSelectionWidget::layerSelectionChanged( int idx )
{
  if ( idx >= 0 )
  {
    QgsMapLayer *layer = QgsProject::instance()->mapLayer( mLayersCombo->itemData( idx ).toString() );
    mLayersCombo->setToolTip( mLayersCombo->itemText( idx ) );
    activateLayer( layer );
    emit selectedLayerChanged( layer );
  }
  else
  {
    mLayersCombo->setToolTip( QString() );
    emit selectedLayerChanged( nullptr );
  }
}

void KadasLayerSelectionWidget::activateLayer( QgsMapLayer *layer )
{
  // A read-only widget reports a layer, it does not pick one: leave the
  // visibility and the current layer alone.
  if ( !layer || mReadOnly || mInitialPopulation )
    return;
  // Whatever is drawn into the selected layer has to be visible, and the rest
  // of the UI follows the current layer. The legend view is optional - map
  // tools living in kadas/gui have no handle on it - so visibility is driven
  // through the project's layer tree, which is the same node either way.
  if ( QgsLayerTreeLayer *node = QgsProject::instance()->layerTreeRoot()->findLayer( layer ) )
    node->setItemVisibilityChecked( true );
  if ( mLayerTreeView )
    mLayerTreeView->setCurrentLayer( layer );
  if ( mCanvas && mCanvas->currentLayer() != layer )
    mCanvas->setCurrentLayer( layer );
}

void KadasLayerSelectionWidget::canvasCurrentLayerChanged( QgsMapLayer *layer )
{
  // Following the legend keeps the tool and the active layer in sync, but only
  // for layers this widget actually offers: making a raster current must not
  // clear the target layer of a tool that is mid-drawing. An unchanged index
  // also means this is the echo of our own activateLayer() call.
  if ( mReadOnly )
    return;
  const int idx = layer ? mLayersCombo->findData( layer->id() ) : -1;
  if ( idx < 0 || idx == mLayersCombo->currentIndex() )
    return;
  mLayersCombo->setCurrentIndex( idx );
}

void KadasLayerSelectionWidget::setSelectedLayer( QgsMapLayer *layer )
{
  int idx = layer ? mLayersCombo->findData( layer->id() ) : -1;
  if ( mReadOnly && layer && idx < 0 )
  {
    // Adding to an empty combo would select the entry behind our back; report
    // the selection once, below.
    mLayersCombo->blockSignals( true );
    mLayersCombo->addItem( QgsIconUtils::iconForLayer( layer ), layer->name(), layer->id() );
    mLayersCombo->blockSignals( false );
    idx = mLayersCombo->count() - 1;
  }
  if ( idx != mLayersCombo->currentIndex() )
  {
    mLayersCombo->setCurrentIndex( idx );
  }
  else
  {
    // Ensure the signal is also emitted when the layer was not found, so the
    // caller learns that its layer is not on offer.
    layerSelectionChanged( idx );
  }
}

QString KadasLayerSelectionWidget::suggestedNewLayerName() const
{
  QString base = mNewLayerName;
  if ( base.isEmpty() )
  {
    if ( QgsMapLayer *current = getSelectedLayer() )
      base = current->name();
  }
  if ( base.isEmpty() )
    base = tr( "Layer" );
  // Suggest "Annotation 3" rather than "Annotation 2 2" when coming off "Annotation 2".
  static const QRegularExpression sTrailingCounter( QStringLiteral( "\\s+\\d+$" ) );
  base.remove( sTrailingCounter );

  QStringList taken;
  const QList<QgsMapLayer *> layers = QgsProject::instance()->mapLayers().values();
  for ( QgsMapLayer *layer : layers )
    taken.append( layer->name() );

  QString name = base;
  for ( int i = 2; taken.contains( name ); ++i )
    name = QStringLiteral( "%1 %2" ).arg( base ).arg( i );
  return name;
}

void KadasLayerSelectionWidget::createLayer()
{
  if ( mReadOnly )
  {
    return;
  }
  bool ok = false;
  const QString layerName = QInputDialog::getText( this, tr( "New Layer" ), tr( "Enter name of new layer:" ), QLineEdit::Normal, suggestedNewLayerName(), &ok );
  if ( !ok || layerName.isEmpty() )
  {
    return;
  }
  QgsMapLayer *layer = mCreator( layerName );
  if ( !layer )
  {
    return;
  }
  QgsProject::instance()->addMapLayer( layer );
  setSelectedLayer( layer );
}
