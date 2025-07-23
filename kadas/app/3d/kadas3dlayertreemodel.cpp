#include "kadas3dlayertreemodel.h"

#include <QComboBox>
#include <QToolButton>
#include <QDoubleSpinBox>
#include <QMenu>
#include <QAction>

#include <qgis/qgslayertree.h>
#include <qgis/qgslayertreemodel.h>
#include <qgis/qgsmaplayer.h>
#include <qgis/qgs3dmapcanvas.h>
#include <qgis/qgs3dmapsettings.h>


const QString Kadas3DLayerTreeModel::TERRAIN_MAP_THEME = QStringLiteral( "3d layers" );


Kadas3DLayerTreeModel::Kadas3DLayerTreeModel( Qgs3DMapCanvas *mapCanvas )
  : QSortFilterProxyModel( mapCanvas )
  , mRoot( QgsProject::instance()->layerTreeRoot() )
  , mMapCanvas( mapCanvas )
{
  visibleLayers( QgsProject::instance()->layerTreeRoot(), mShownLayers );

  setLayerTreeModel( new QgsLayerTreeModel( QgsProject::instance()->layerTreeRoot(), this ) );

  connect( QgsProject::instance(), &QgsProject::readProject, this, [=] {
    beginResetModel();
    mShownLayers.clear();
    visibleLayers( QgsProject::instance()->layerTreeRoot(), mShownLayers );
    endResetModel();
  } );
}

int Kadas3DLayerTreeModel::columnCount( const QModelIndex &parent ) const
{
  Q_UNUSED( parent )
  return 1;
}

Qt::ItemFlags Kadas3DLayerTreeModel::flags( const QModelIndex &idx ) const
{
  if ( idx.column() == 0 )
  {
    if ( !mapLayer( idx ) )
      return Qt::ItemIsEnabled;

    // if this is a layer, allow check state
    return Qt::ItemIsEnabled | Qt::ItemIsUserCheckable;
  }

  return Qt::NoItemFlags;
}

QModelIndex Kadas3DLayerTreeModel::index( int row, int column, const QModelIndex &parent ) const
{
  if ( row < 0 || column < 0 || row >= rowCount( parent ) || column >= columnCount( parent ) )
  {
    return QModelIndex();
  }

  QModelIndex newIndex = QSortFilterProxyModel::index( row, 0, parent );
  if ( column == 0 )
    return newIndex;

  return createIndex( row, column, newIndex.internalId() );
}

QModelIndex Kadas3DLayerTreeModel::parent( const QModelIndex &child ) const
{
  return QSortFilterProxyModel::parent( createIndex( child.row(), 0, child.internalId() ) );
}

QModelIndex Kadas3DLayerTreeModel::sibling( int row, int column, const QModelIndex &idx ) const
{
  const QModelIndex parent = idx.parent();
  return index( row, column, parent );
}

QgsMapLayer *Kadas3DLayerTreeModel::mapLayer( const QModelIndex &idx ) const
{
  QgsLayerTreeNode *node = nullptr;
  if ( idx.column() == 0 )
  {
    node = mLayerTreeModel->index2node( mapToSource( idx ) );
  }
  else
  {
    node = mLayerTreeModel->index2node( mapToSource( index( idx.row(), 0, idx.parent() ) ) );
  }

  if ( !node || !QgsLayerTree::isLayer( node ) )
    return nullptr;

  return QgsLayerTree::toLayer( node )->layer();
}

void Kadas3DLayerTreeModel::setFilterText( const QString &filterText )
{
  if ( filterText == mFilterText )
    return;

  mFilterText = filterText;
  invalidateFilter();
}

QgsLayerTreeModel *Kadas3DLayerTreeModel::layerTreeModel() const
{
  return mLayerTreeModel;
}

void Kadas3DLayerTreeModel::setLayerTreeModel( QgsLayerTreeModel *layerTreeModel )
{
  mLayerTreeModel = layerTreeModel;
  QSortFilterProxyModel::setSourceModel( layerTreeModel );
}

bool Kadas3DLayerTreeModel::filterAcceptsRow( int sourceRow, const QModelIndex &sourceParent ) const
{
  QgsLayerTreeNode *node = mLayerTreeModel->index2node( mLayerTreeModel->index( sourceRow, 0, sourceParent ) );
  return nodeShown( node );
}

bool Kadas3DLayerTreeModel::nodeShown( QgsLayerTreeNode *node ) const
{
  if ( !node )
    return false;
  if ( node->nodeType() == QgsLayerTreeNode::NodeGroup )
  {
    const auto constChildren = node->children();
    for ( QgsLayerTreeNode *child : constChildren )
    {
      if ( nodeShown( child ) )
      {
        return true;
      }
    }
    return false;
  }
  else
  {
    QgsMapLayer *layer = QgsLayerTree::toLayer( node )->layer();
    return layer && layer->isSpatial() && ( mFilterText.isEmpty() || layer->name().contains( mFilterText, Qt::CaseInsensitive ) );
  }
}

QVariant Kadas3DLayerTreeModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
  if ( orientation == Qt::Horizontal )
  {
    if ( role == Qt::DisplayRole )
    {
      switch ( section )
      {
        case 0:
          return tr( "Layer" );
        default:
          return QVariant();
      }
    }
  }
  return mLayerTreeModel->headerData( section, orientation, role );
}

QVariant Kadas3DLayerTreeModel::data( const QModelIndex &idx, int role ) const
{
  if ( idx.column() == 0 )
  {
    if ( static_cast<Qt::ItemDataRole>( role ) == Qt::ItemDataRole::CheckStateRole )
    {
      QgsMapLayer *layer = mapLayer( idx );
      if ( layer )
      {
        return mShownLayers.contains( layer ) ? Qt::Checked : Qt::Unchecked;
      }
    }
    else
    {
      return mLayerTreeModel->data( mapToSource( idx ), role );
    }
  }

  return QVariant();
}

bool Kadas3DLayerTreeModel::setData( const QModelIndex &index, const QVariant &value, int role )
{
  if ( index.column() == 0 )
  {
    if ( static_cast<Qt::ItemDataRole>( role ) == Qt::ItemDataRole::CheckStateRole )
    {
      QgsMapLayer *layer = mapLayer( index );
      if ( !layer )
        return false;

      if ( value.value<Qt::CheckState>() == Qt::Checked )
        mShownLayers.insert( layer );
      else if ( value.value<Qt::CheckState>() == Qt::Unchecked )
        mShownLayers.remove( layer );
      else
        Q_ASSERT( false ); // expected checked or unchecked

      emit dataChanged( index, index );

      // create a new map theme and use it 3D
      QgsMapThemeCollection::MapThemeRecord record;
      createThemeFromCurrentState( mRoot, record );

      if ( !mMapCanvas->mapSettings()->mapThemeCollection()->hasMapTheme( TERRAIN_MAP_THEME ) )
      {
        QgsProject::instance()->mapThemeCollection()->insert( TERRAIN_MAP_THEME, record );
        mMapCanvas->mapSettings()->setTerrainMapTheme( TERRAIN_MAP_THEME );
      }
      else
      {
        mMapCanvas->mapSettings()->mapThemeCollection()->update( TERRAIN_MAP_THEME, record );
      }

      mMapCanvas->mapSettings()->setLayers( mShownLayers.values() );

      return true;
    }

    return mLayerTreeModel->setData( mapToSource( index ), value, role );
  }

  return false;
}

void Kadas3DLayerTreeModel::createThemeFromCurrentState( QgsLayerTreeGroup *parent, QgsMapThemeCollection::MapThemeRecord &rec )
{
  const QList<QgsLayerTreeNode *> constChildren = parent->children();
  for ( QgsLayerTreeNode *node : constChildren )
  {
    if ( QgsLayerTree::isGroup( node ) )
    {
      createThemeFromCurrentState( QgsLayerTree::toGroup( node ), rec );
    }
    else if ( QgsLayerTree::isLayer( node ) )
    {
      QgsLayerTreeLayer *nodeLayer = QgsLayerTree::toLayer( node );
      if ( nodeLayer->layer() && mShownLayers.contains( nodeLayer->layer() ) )
      {
        QgsMapThemeCollection::MapThemeLayerRecord layerRecord( nodeLayer->layer() );
        rec.addLayerRecord( layerRecord );
      }
    }
  }
}

void Kadas3DLayerTreeModel::visibleLayers( QgsLayerTreeGroup *parent, QSet<QgsMapLayer *> &layers )
{
  const QList<QgsLayerTreeNode *> constChildren = parent->children();
  for ( QgsLayerTreeNode *node : constChildren )
  {
    if ( QgsLayerTree::isGroup( node ) )
    {
      visibleLayers( QgsLayerTree::toGroup( node ), layers );
    }
    else if ( QgsLayerTree::isLayer( node ) )
    {
      QgsLayerTreeLayer *nodeLayer = QgsLayerTree::toLayer( node );
      if ( node->isVisible() != Qt::Unchecked && nodeLayer->layer() )
      {
        layers << nodeLayer->layer();
      }
    }
  }
}
