/***************************************************************************
    kadassearchbox.cpp
    ------------------
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

#include <QCheckBox>
#include <QHeaderView>
#include <QImageReader>
#include <QKeyEvent>
#include <QLineEdit>
#include <QMenu>
#include <QShortcut>
#include <QStyle>
#include <QToolButton>
#include <QTreeWidgetItem>
#include <QVBoxLayout>

#include <qgis/qgscoordinatetransform.h>
#include <qgis/qgsmapcanvas.h>
#include <qgis/qgspolygon.h>
#include <qgis/qgsproject.h>

#include <kadas/gui/kadasmapcanvasitemmanager.h>
#include <kadas/gui/kadassearchbox.h>
#include <kadas/gui/kadassearchprovider.h>
#include <kadas/gui/mapitems/kadascircleitem.h>
#include <kadas/gui/mapitems/kadassymbolitem.h>
#include <kadas/gui/mapitems/kadaspolygonitem.h>
#include <kadas/gui/mapitems/kadasrectangleitem.h>
#include <kadas/gui/maptools/kadasmaptoolcreateitem.h>



const int KadasSearchBox::sEntryTypeRole = Qt::UserRole;
const int KadasSearchBox::sCatNameRole = Qt::UserRole + 1;
const int KadasSearchBox::sCatPrecedenceRole = Qt::UserRole + 2;
const int KadasSearchBox::sCatCountRole = Qt::UserRole + 3;
const int KadasSearchBox::sResultDataRole = Qt::UserRole + 4;

// Overridden to make event() public
class KadasSearchBox::LineEdit : public QLineEdit
{
  public:
    LineEdit( QWidget *parent ) : QLineEdit( parent ) {}
    bool event( QEvent *e ) override { return QLineEdit::event( e ); }
};


// Overridden to make event() public
class KadasSearchBox::TreeWidget: public QTreeWidget
{
  public:
    TreeWidget( QWidget *parent ) : QTreeWidget( parent ) {}
    bool event( QEvent *e ) override { return QTreeWidget::event( e ); }
};


KadasSearchBox::KadasSearchBox( QWidget *parent )
  : QWidget( parent )
{ }

void KadasSearchBox::init( QgsMapCanvas *canvas )
{
  mMapCanvas = canvas;
  mNumRunningProviders = 0;
  mPin = nullptr;
  mFilterTool = nullptr;

  mSearchBox = new LineEdit( this );
  mSearchBox->setObjectName( "searchBox" );
  mSearchBox->setSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::Preferred );

  mTreeWidget = new TreeWidget( mSearchBox );
  mTreeWidget->setWindowFlags( Qt::Popup );
  mTreeWidget->setFocusPolicy( Qt::NoFocus );
  mTreeWidget->setFrameStyle( QFrame::Box );
  mTreeWidget->setRootIsDecorated( true );
  mTreeWidget->setColumnCount( 1 );
  mTreeWidget->setEditTriggers( QTreeWidget::NoEditTriggers );
  mTreeWidget->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
  mTreeWidget->setMouseTracking( true );
  mTreeWidget->setUniformRowHeights( true );
  mTreeWidget->header()->hide();
  mTreeWidget->hide();

  mTimer.setSingleShot( true );
  mTimer.setInterval( 500 );

  mSearchButton = new QToolButton( mSearchBox );
  mSearchButton->setIcon( QIcon( ":/kadas/icons/search" ) );
  mSearchButton->setIconSize( QSize( 16, 16 ) );
  mSearchButton->setCursor( Qt::PointingHandCursor );
  mSearchButton->setStyleSheet( "QToolButton { border: none; padding: 0px; }" );
  mSearchButton->setToolTip( tr( "Search" ) );

  mClearButton = new QToolButton( mSearchBox );
  mClearButton->setIcon( QIcon( ":/kadas/icons/reset" ) );
  mClearButton->setIconSize( QSize( 16, 16 ) );
  mClearButton->setCursor( Qt::PointingHandCursor );
  mClearButton->setStyleSheet( "QToolButton { border: none; padding: 0px; }" );
  mClearButton->setToolTip( tr( "Clear" ) );
  mClearButton->setVisible( false );
  mClearButton->installEventFilter( this );

  QMenu *filterMenu = new QMenu( mSearchBox );
  QActionGroup *filterActionGroup = new QActionGroup( filterMenu );
  QAction *noFilterAction = new QAction( QIcon( ":/kadas/icons/search_filter_none" ), tr( "No filter" ), filterMenu );
  filterActionGroup->addAction( noFilterAction );
  connect( noFilterAction, &QAction::triggered, this, &KadasSearchBox::clearFilter );

  QAction *circleFilterAction = new QAction( QIcon( ":/kadas/icons/search_filter_circle" ), tr( "Filter by radius" ), filterMenu );
  circleFilterAction->setData( QVariant::fromValue( static_cast<int>( FilterCircle ) ) );
  filterActionGroup->addAction( circleFilterAction );
  connect( circleFilterAction, &QAction::triggered, this, &KadasSearchBox::setFilterTool );

  QAction *rectangleFilterAction = new QAction( QIcon( ":/kadas/icons/search_filter_rect" ), tr( "Filter by rectangle" ), filterMenu );
  rectangleFilterAction->setData( QVariant::fromValue( static_cast<int>( FilterRect ) ) );
  filterActionGroup->addAction( rectangleFilterAction );
  connect( rectangleFilterAction, &QAction::triggered, this, &KadasSearchBox::setFilterTool );

  QAction *polygonFilterAction = new QAction( QIcon( ":/kadas/icons/search_filter_poly" ), tr( "Filter by polygon" ), filterMenu );
  polygonFilterAction->setData( QVariant::fromValue( static_cast<int>( FilterPoly ) ) );
  filterActionGroup->addAction( polygonFilterAction );
  connect( polygonFilterAction, &QAction::triggered, this, &KadasSearchBox::setFilterTool );

  filterMenu->addActions( QList<QAction *>() << noFilterAction << circleFilterAction << rectangleFilterAction << polygonFilterAction );

  mFilterButton = new QToolButton( this );
  mFilterButton->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::MinimumExpanding );
  mFilterButton->setObjectName( "searchFilter" );
  mFilterButton->setDefaultAction( noFilterAction );
  mFilterButton->setIconSize( QSize( 16, 16 ) );
  mFilterButton->setPopupMode( QToolButton::InstantPopup );
  mFilterButton->setCursor( Qt::PointingHandCursor );
  mFilterButton->setToolTip( tr( "Select Filter" ) );
  mFilterButton->setMenu( filterMenu );
  connect( filterMenu, &QMenu::triggered, mFilterButton, &QToolButton::setDefaultAction );

  setLayout( new QHBoxLayout );
  layout()->addWidget( mSearchBox );
  layout()->addWidget( mFilterButton );
  layout()->setContentsMargins( 0, 5, 0, 5 );
  layout()->setSpacing( 0 );

  connect( mSearchBox, &LineEdit::textEdited, this, &KadasSearchBox::textChanged );
  connect( mSearchButton, &QToolButton::clicked, this, &KadasSearchBox::startSearch );
  connect( &mTimer, &QTimer::timeout, this, &KadasSearchBox::startSearch );
  connect( mTreeWidget, &TreeWidget::itemSelectionChanged, this, &KadasSearchBox::resultSelected );
  connect( mTreeWidget, &TreeWidget::itemClicked, this, &KadasSearchBox::resultActivated );
  connect( mTreeWidget, &TreeWidget::itemActivated, this, &KadasSearchBox::resultActivated );
  connect( QgsProject::instance(), &QgsProject::cleared, this, &KadasSearchBox::clearSearch );

  int frameWidth = mSearchBox->style()->pixelMetric( QStyle::PM_DefaultFrameWidth );
  mSearchBox->setStyleSheet( QString( "QLineEdit { padding-right: %1px; } " ).arg( mSearchButton->sizeHint().width() + frameWidth + 5 ) );
  QSize msz = mSearchBox->minimumSizeHint();
  mSearchBox->setMinimumSize( std::max( msz.width(), mSearchButton->sizeHint().height() + frameWidth * 2 + 2 ),
                              std::max( msz.height(), mSearchButton->sizeHint().height() + frameWidth * 2 + 2 ) );
  mSearchBox->setPlaceholderText( tr( "Search for Places, Coordinates, Adresses, ..." ) );

  qRegisterMetaType<KadasSearchProvider::SearchResult> ( "KadasSearchProvider::SearchResult" );

  mSearchBox->installEventFilter( this );
  mTreeWidget->installEventFilter( this );

  QShortcut *shortcut = new QShortcut( QKeySequence( Qt::CTRL + Qt::Key_F ), this );
  QObject::connect( shortcut, &QShortcut::activated, mSearchBox, qOverload<> ( &LineEdit::setFocus ) );

}

KadasSearchBox::~KadasSearchBox()
{
  clearSearch();
  qDeleteAll( mSearchProviders );
}

void KadasSearchBox::addSearchProvider( KadasSearchProvider *provider )
{
  mSearchProviders.append( provider );
  connect( provider, &KadasSearchProvider::searchFinished, this, &KadasSearchBox::searchProviderFinished );
  connect( provider, &KadasSearchProvider::searchResultFound, this, &KadasSearchBox::searchResultFound );
}

void KadasSearchBox::removeSearchProvider( KadasSearchProvider *provider )
{
  mSearchProviders.removeAll( provider );
  disconnect( provider, &KadasSearchProvider::searchFinished, this, &KadasSearchBox::searchProviderFinished );
  disconnect( provider, &KadasSearchProvider::searchResultFound, this, &KadasSearchBox::searchResultFound );
}

bool KadasSearchBox::eventFilter( QObject *obj, QEvent *ev )
{
  if ( obj == mTreeWidget && ev->type() == QEvent::MouseButtonPress )
  {
    QMouseEvent *mev = static_cast<QMouseEvent *>( ev );
    if ( mSearchBox->rect().contains( mSearchBox->mapFromGlobal( mev->globalPos() ) ) )
    {
      mSearchBox->event( ev );
    }
    else
    {
      mTreeWidget->close();
    }
    return true;
  }
  else if ( obj == mTreeWidget && ( ev->type() == QEvent::MouseMove || ev->type() == QEvent::MouseButtonRelease ) )
  {
    QMouseEvent *mev = static_cast<QMouseEvent *>( ev );
    if ( mSearchBox->rect().contains( mSearchBox->mapFromGlobal( mev->globalPos() ) ) )
    {
      mSearchBox->event( ev );
      return true;
    }
  }
  else if ( obj == mSearchBox && ev->type() == QEvent::FocusIn )
  {
    mTreeWidget->resize( mSearchBox->width(), 200 );
    mTreeWidget->move( mSearchBox->mapToGlobal( QPoint( 0, mSearchBox->height() ) ) );
    mTreeWidget->show();
    mMapCanvas->setMapTool( mFilterTool );
    if ( !mClearButton->isVisible() )
    {
      resultSelected();
    }
    if ( mFilterItem )
    {
      KadasMapCanvasItemManager::addItem( mFilterItem );
    }
    return true;
  }
  else if ( obj == mSearchBox && ev->type() == QEvent::MouseButtonPress )
  {
    mSearchBox->selectAll();
    return true;
  }
  else if ( obj == mSearchBox && ev->type() == QEvent::Resize )
  {
    int frameWidth = mSearchBox->style()->pixelMetric( QStyle::PM_DefaultFrameWidth );
    QRect r = mSearchBox->rect();
    QSize sz = mSearchButton->sizeHint();
    mSearchButton->move( ( r.right() - frameWidth - sz.width() - 4 ),
                         ( r.bottom() + 1 - sz.height() ) / 2 );
    sz = mClearButton->sizeHint();
    mClearButton->move( ( r.right() - frameWidth - sz.width() - 4 ),
                        ( r.bottom() + 1 - sz.height() ) / 2 );
    return true;
  }
  else if ( obj == mClearButton && ev->type() == QEvent::MouseButtonPress )
  {
    clearSearch();
    return true;
  }
  else if ( obj == mTreeWidget && ev->type() == QEvent::Close )
  {
    cancelSearch();
    mSearchBox->clearFocus();
    if ( mFilterItem )
    {
      KadasMapCanvasItemManager::removeItem( mFilterItem );
    }
    return true;
  }
  else if ( obj == mTreeWidget && ev->type() == QEvent::KeyPress )
  {
    int key = static_cast<QKeyEvent *>( ev )->key();
    if ( key == Qt::Key_Escape )
    {
      mTreeWidget->close();
      return true;
    }
    else if ( key == Qt::Key_Enter || key == Qt::Key_Return )
    {
      if ( mTimer.isActive() )
      {
        // Search text was changed
        startSearch();
        return true;
      }
    }
    else if ( key == Qt::Key_Up || key == Qt::Key_Down || key == Qt::Key_PageUp || key == Qt::Key_PageDown )
    {
      return mTreeWidget->event( ev );
    }
    else
    {
      return mSearchBox->event( ev );
    }
  }
  return QWidget::eventFilter( obj, ev );
}

void KadasSearchBox::textChanged()
{
  mSearchButton->setVisible( true );
  mClearButton->setVisible( false );
  cancelSearch();
  mTimer.start();
}

void KadasSearchBox::startSearch()
{
  mTimer.stop();

  mTreeWidget->blockSignals( true );
  mTreeWidget->clear();
  mTreeWidget->blockSignals( false );

  QString searchtext = mSearchBox->text().trimmed();
  if ( searchtext.size() < 3 )
  {
    return;
  }

  mNumRunningProviders = mSearchProviders.count();

  KadasSearchProvider::SearchRegion searchRegion;
  if ( mFilterTool )
  {
    QgsPolygonXY poly;
    const KadasMapToolCreateItem *filterTool = mFilterTool;
    if ( filterTool->currentItem() && dynamic_cast<const KadasGeometryItem *>( filterTool->currentItem() ) )
    {
      poly = QgsGeometry( static_cast<const KadasGeometryItem *>( filterTool->currentItem() )->geometry()->clone() ).asPolygon();
    }
    if ( !poly.isEmpty() )
    {
      searchRegion.polygon = poly.front();
      searchRegion.crs = mMapCanvas->mapSettings().destinationCrs().authid();
    }
  }

  for ( KadasSearchProvider *provider : mSearchProviders )
  {
    provider->startSearch( searchtext, searchRegion );
  }
}

void KadasSearchBox::clearSearch()
{
  mSearchBox->clear();
  mSearchButton->setVisible( true );
  mClearButton->setVisible( false );
  clearPin();
  mTreeWidget->close();
  mTreeWidget->blockSignals( true );
  mTreeWidget->clear();
  mTreeWidget->blockSignals( false );
}

void KadasSearchBox::searchProviderFinished()
{
  --mNumRunningProviders;
  if ( mNumRunningProviders == 0 && mTreeWidget->invisibleRootItem()->childCount() == 0 )
  {
    QTreeWidgetItem *noResultsItem = new QTreeWidgetItem();
    noResultsItem->setData( 0, Qt::DisplayRole, tr( "No search results found" ) );
    noResultsItem->setFlags( noResultsItem->flags() & ~Qt::ItemIsEnabled );
    mTreeWidget->invisibleRootItem()->insertChild( 0, noResultsItem );
  }
}

void KadasSearchBox::searchResultFound( KadasSearchProvider::SearchResult result )
{
  // If result is fuzzy, search for fuzzy category
  QTreeWidgetItem *root = mTreeWidget->invisibleRootItem();
  if ( result.fuzzy )
  {
    int n = root->childCount();
    if ( n == 0 || root->child( n - 1 )->data( 0, sCatNameRole ).toString() != "fuzzy" )
    {
      QTreeWidgetItem *fuzzyItem = new QTreeWidgetItem();
      fuzzyItem->setData( 0, sEntryTypeRole, EntryTypeCategory );
      fuzzyItem->setData( 0, sCatNameRole, "fuzzy" );
      fuzzyItem->setData( 0, sCatPrecedenceRole, 100000 );
      fuzzyItem->setData( 0, Qt::DisplayRole, tr( "Close matches" ) );
      fuzzyItem->setFlags( Qt::ItemIsEnabled );
      QFont font = fuzzyItem->font( 0 );
      font.setBold( true );
      font.setItalic( true );
      fuzzyItem->setFont( 0, font );
      root->addChild( fuzzyItem );
      fuzzyItem->setExpanded( true );
    }
    root = root->child( root->childCount() - 1 );
  }
  // Search category item
  QTreeWidgetItem *categoryItem = 0;
  for ( int i = 0, n = root->childCount(); i < n; ++i )
  {
    if ( root->child( i )->data( 0, sCatNameRole ).toString() == result.category )
    {
      categoryItem = root->child( i );
    }
  }

  // If category does not exist, create it
  if ( !categoryItem )
  {
    int pos = 0;
    for ( int i = 0, n = root->childCount(); i < n; ++i )
    {
      if ( result.categoryPrecedence < root->child( i )->data( 0, sCatPrecedenceRole ).toInt() )
      {
        break;
      }
      ++pos;
    }
    categoryItem = new QTreeWidgetItem();
    categoryItem->setData( 0, sEntryTypeRole, EntryTypeCategory );
    categoryItem->setData( 0, sCatNameRole, result.category );
    categoryItem->setData( 0, sCatPrecedenceRole, result.categoryPrecedence );
    categoryItem->setData( 0, sCatCountRole, 0 );
    categoryItem->setFlags( Qt::ItemIsEnabled );
    QFont font = categoryItem->font( 0 );
    font.setBold( true );
    categoryItem->setFont( 0, font );
    root->insertChild( pos, categoryItem );
    categoryItem->setExpanded( true );
  }

  // Insert new result
  QTreeWidgetItem *resultItem = new QTreeWidgetItem();
  resultItem->setData( 0, Qt::DisplayRole, result.text );
  resultItem->setData( 0, sEntryTypeRole, EntryTypeResult );
  resultItem->setData( 0, sResultDataRole, QVariant::fromValue( result ) );
  resultItem->setFlags( Qt::ItemIsSelectable | Qt::ItemIsEnabled );

  categoryItem->addChild( resultItem );
  int categoryCount = categoryItem->data( 0, sCatCountRole ).toInt() + 1;
  categoryItem->setData( 0, sCatCountRole, categoryCount );
  categoryItem->setData( 0, Qt::DisplayRole, QString( "%1 (%2)" ).arg( categoryItem->data( 0, sCatNameRole ).toString() ).arg( categoryCount ) );
}

void KadasSearchBox::resultSelected()
{
  if ( mTreeWidget->currentItem() )
  {
    QTreeWidgetItem *item = mTreeWidget->currentItem();
    if ( item->data( 0, sEntryTypeRole ) != EntryTypeResult )
    {
      clearPin();
      return;
    }

    KadasSearchProvider::SearchResult result = item->data( 0, sResultDataRole ).value<KadasSearchProvider::SearchResult>();
    if ( result.showPin )
    {
      if ( !mPin )
      {
        mPin = new KadasSymbolItem( mMapCanvas->mapSettings().destinationCrs() );
        mPin->setup( ":/kadas/icons/pin_blue", 0.5, 1.0 );
        KadasMapCanvasItemManager::addItem( mPin );
      }
      QgsPointXY itemPos = QgsCoordinateTransform( QgsCoordinateReferenceSystem( result.crs ), mPin->crs(), QgsProject::instance() ).transform( result.pos );
      mPin->setPosition( KadasItemPos::fromPoint( itemPos ) );
    }
    else
    {
      clearPin();
    }
    mSearchBox->blockSignals( true );
    mSearchBox->setText( result.text );
    mSearchBox->blockSignals( false );
    mSearchButton->setVisible( true );
    mClearButton->setVisible( false );
  }
  else
  {
    clearPin();
  }
}

void KadasSearchBox::resultActivated()
{
  if ( mTreeWidget->currentItem() )
  {
    QTreeWidgetItem *item = mTreeWidget->currentItem();
    if ( item->data( 0, sEntryTypeRole ) != EntryTypeResult )
    {
      return;
    }

    KadasSearchProvider::SearchResult result = item->data( 0, sResultDataRole ).value<KadasSearchProvider::SearchResult>();
    QgsRectangle zoomExtent;
    if ( result.bbox.isEmpty() )
    {
      zoomExtent = mMapCanvas->mapSettings().computeExtentForScale( result.pos, result.zoomScale, QgsCoordinateReferenceSystem( result.crs ) );
    }
    else
    {
      QgsCoordinateTransform t( QgsCoordinateReferenceSystem( result.crs ), mMapCanvas->mapSettings().destinationCrs(), QgsProject::instance() );
      zoomExtent = t.transform( result.bbox );
    }
    mMapCanvas->setExtent( zoomExtent, true );
    mMapCanvas->refresh();
    mSearchBox->blockSignals( true );
    mSearchBox->setText( result.text );
    mSearchBox->blockSignals( false );
    mSearchButton->setVisible( false );
    mClearButton->setVisible( true );
    mTreeWidget->close();
  }
}

void KadasSearchBox::cancelSearch()
{
  for ( KadasSearchProvider *provider : mSearchProviders )
  {
    provider->cancelSearch();
  }
  // If the clear button is visible, the pin marks an activated search
  // result, which can be cleared by pressing the clear button
  if ( !mClearButton->isVisible() )
  {
    clearPin();
  }
}

void KadasSearchBox::clearPin()
{

  if ( mPin )
  {
    KadasMapCanvasItemManager::removeItem( mPin );
    delete mPin;
    mPin = nullptr;
  }
}

void KadasSearchBox::clearFilter()
{
  if ( mFilterItem )
  {
    delete mFilterItem;
    mFilterItem = nullptr;
    // Trigger a new search since the filter changed
    startSearch();
  }
  if ( mFilterTool )
  {
    mMapCanvas->unsetMapTool( mFilterTool );
    mFilterTool = nullptr;
  }
}

void KadasSearchBox::setFilterTool()
{
  clearFilter();
  QAction *action = qobject_cast<QAction *> ( QObject::sender() );
  FilterType filterType = static_cast<FilterType>( action->data().toInt() );
  KadasMapToolCreateItem::ItemFactory factory = nullptr;
  switch ( filterType )
  {
    case FilterRect:
      factory = [ = ] { return new KadasRectangleItem( mMapCanvas->mapSettings().destinationCrs() ); };
      break;
    case FilterPoly:
      factory = [ = ] { return new KadasPolygonItem( mMapCanvas->mapSettings().destinationCrs() ); };
      break;
    case FilterCircle:
      factory = [ = ] { return new KadasCircleItem( mMapCanvas->mapSettings().destinationCrs() ); };
      break;
  }
  if ( factory )
  {
    mFilterTool = new KadasMapToolCreateItem( mMapCanvas, factory );
    mMapCanvas->setMapTool( mFilterTool );
    action->setCheckable( true );
    action->setChecked( true );
    connect( mFilterTool, &KadasMapToolCreateItem::partFinished, this, &KadasSearchBox::filterToolFinished );
  }
}

void KadasSearchBox::filterToolFinished()
{
  mFilterItem = mFilterTool->takeItem();
  mFilterButton->defaultAction()->setChecked( false );
  mFilterButton->defaultAction()->setCheckable( false );
  mMapCanvas->unsetMapTool( mFilterTool );
  mFilterTool = nullptr;
  mSearchBox->setFocus();
  mSearchBox->selectAll();
  // Trigger a new search since the filter changed
  startSearch();
}
