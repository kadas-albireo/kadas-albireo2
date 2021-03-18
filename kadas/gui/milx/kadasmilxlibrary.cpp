/***************************************************************************
    kadasmilxlibrary.cpp
    --------------------
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
#include <QDir>
#include <QDomDocument>
#include <QPainter>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>
#include <QTreeView>
#include <QVBoxLayout>
#include <QtConcurrentRun>

#include <qgis/qgsfilterlineedit.h>
#include <qgis/qgssettings.h>

#include <kadas/gui/milx/kadasmilxlibrary.h>

const int KadasMilxLibrary::SymbolXmlRole = Qt::UserRole + 1;
const int KadasMilxLibrary::SymbolMilitaryNameRole = Qt::UserRole + 2;
const int KadasMilxLibrary::SymbolPointCountRole = Qt::UserRole + 3;
const int KadasMilxLibrary::SymbolVariablePointsRole = Qt::UserRole + 4;


class KadasMilxLibrary::TreeFilterProxyModel : public QSortFilterProxyModel
{
  public:
    TreeFilterProxyModel( QObject *parent = 0 ) : QSortFilterProxyModel( parent )
    {
    }

    bool filterAcceptsRow( int source_row, const QModelIndex &source_parent ) const override
    {
      if ( QSortFilterProxyModel::filterAcceptsRow( source_row, source_parent ) )
      {
        return true;
      }
      QModelIndex parent = source_parent;
      while ( parent.isValid() )
      {
        if ( QSortFilterProxyModel::filterAcceptsRow( parent.row(), parent.parent() ) )
        {
          return true;
        }
        parent = parent.parent();
      }
      return acceptsAnyChildren( source_row, source_parent );
    }

  private:
    bool acceptsAnyChildren( int source_row, const QModelIndex &source_parent ) const
    {
      QModelIndex item = sourceModel()->index( source_row, 0, source_parent );
      if ( item.isValid() )
      {
        for ( int i = 0, n = item.model()->rowCount( item ); i < n; ++i )
        {
          if ( QSortFilterProxyModel::filterAcceptsRow( i, item ) || acceptsAnyChildren( i, item ) )
            return true;
        }
      }
      return false;
    }
};


KadasMilxLibrary::KadasMilxLibrary( WId winId, QWidget *parent )
  : QFrame( parent ), mWinId( winId )
{
  setWindowFlags( Qt::Popup );
  setFrameShape( QFrame::Panel );
  setFrameStyle( QFrame::Plain );
  setLineWidth( 1 );

  QVBoxLayout *layout = new QVBoxLayout( this );
  layout->setMargin( 2 );
  layout->setSpacing( 2 );
  setLayout( layout );

  mFilterLineEdit = new QgsFilterLineEdit( this );
  mFilterLineEdit->setPlaceholderText( tr( "Filter..." ) );
  mFilterLineEdit->setEnabled( false );
  layout->addWidget( mFilterLineEdit );
  connect( mFilterLineEdit, &QLineEdit::textChanged, this, &KadasMilxLibrary::filterChanged );

  mTreeView = new QTreeView( this );
  mTreeView->setFrameShape( QTreeView::NoFrame );
  mTreeView->setEditTriggers( QTreeView::NoEditTriggers );
  mTreeView->setHeaderHidden( true );
  mTreeView->setIconSize( QSize( 32, 32 ) );
  layout->addWidget( mTreeView );
  connect( mTreeView, &QTreeView::clicked, this, &KadasMilxLibrary::itemClicked );

  mLoadingModel = new QStandardItemModel( this );
  QStandardItem *loadingItem = new QStandardItem( tr( "Loading..." ) );
  loadingItem->setEnabled( false );
  mLoadingModel->appendRow( loadingItem );

  setCursor( Qt::WaitCursor );
  mTreeView->setModel( mLoadingModel );

  mLibraryFuture.setFuture( QtConcurrent::run( [this] { return loadLibrary( mTreeView->iconSize() ); } ) );
  connect( &mLibraryFuture, &QFutureWatcher<QStandardItemModel *>::finished, this, &KadasMilxLibrary::loaderFinished );
}

KadasMilxLibrary::~KadasMilxLibrary()
{
  mLoaderAborted = 1;
  mLibraryFuture.cancel();
  mLibraryFuture.waitForFinished();
}

void KadasMilxLibrary::focusFilter()
{
  mFilterLineEdit->setFocus();
  mFilterLineEdit->selectAll();
}

void KadasMilxLibrary::loaderFinished()
{
  mGalleryModel = mLibraryFuture.result();
  mGalleryModel->setParent( this );
  mFilterProxyModel = new TreeFilterProxyModel( this );
  mFilterProxyModel->setSourceModel( mGalleryModel );
  mTreeView->setModel( mFilterProxyModel );
  mFilterLineEdit->setEnabled( true );
  delete mLoadingModel;
  unsetCursor();
}

void KadasMilxLibrary::filterChanged( const QString &text )
{
  mTreeView->clearSelection();
  mFilterProxyModel->setFilterFixedString( text );
  mFilterProxyModel->setFilterCaseSensitivity( Qt::CaseInsensitive );
  if ( text.length() >= 3 )
  {
    mTreeView->expandAll();
  }
}

void KadasMilxLibrary::itemClicked( const QModelIndex &index )
{

  QModelIndex sourceIndex = mFilterProxyModel->mapToSource( index );
  QList<QModelIndex> indexStack;
  indexStack.prepend( sourceIndex );
  while ( sourceIndex.parent().isValid() )
  {
    sourceIndex = sourceIndex.parent();
    indexStack.prepend( sourceIndex );
  }

  QStandardItem *item = mGalleryModel->itemFromIndex( indexStack.front() );
  if ( item )
  {
    for ( int i = 1, n = indexStack.size(); i < n; ++i )
    {
      item = item->child( indexStack[i].row() );
    }

    KadasMilxSymbolDesc symbolDesc;
    symbolDesc.symbolXml = item->data( SymbolXmlRole ).toString();
    if ( symbolDesc.symbolXml.isEmpty() )
    {
      return;
    }
    hide();
    if ( symbolDesc.symbolXml == "<custom>" )
    {
      if ( !KadasMilxClient::createSymbol( symbolDesc.symbolXml, symbolDesc, mWinId ) )
      {
        return;
      }
    }
    else
    {
      symbolDesc.militaryName = item->data( SymbolMilitaryNameRole ).toString();
      symbolDesc.minNumPoints = item->data( SymbolPointCountRole ).toInt();
      symbolDesc.hasVariablePoints = item->data( SymbolVariablePointsRole ).toInt();
      symbolDesc.icon = item->icon().pixmap( item->icon().actualSize( QSize( 32, 32 ) ) ).toImage();
    }
    emit symbolSelected( symbolDesc );
  }
}

QStandardItemModel *KadasMilxLibrary::loadLibrary( const QSize &viewIconSize )
{
  QStandardItemModel *model = new QStandardItemModel();

#ifdef __MINGW32__
  QString galleryPath = QDir( QString( "%1/../opt/mss/MilXGalleryFiles" ).arg( QApplication::applicationDirPath() ) ).absolutePath();
#else
  QString galleryPath = QDir( QApplication::applicationDirPath() ).absoluteFilePath( "MilXGalleryFiles" );
#endif
  if ( !QDir( galleryPath ).exists() )
  {
    galleryPath = QgsSettings().value( "/milx/milx_gallery_path", "" ).toString();
  }
  QString lang = QgsSettings().value( "/locale/userLocale", "en" ).toString().left( 2 ).toUpper();

  QDir galleryDir( galleryPath );
  if ( galleryDir.exists() )
  {
    for ( const QString &galleryFileName : galleryDir.entryList( QStringList() << "*.xml", QDir::Files ) )
    {
      QString galleryFilePath = galleryDir.absoluteFilePath( galleryFileName );
      QFile galleryFile( galleryFilePath );
      if ( !galleryFilePath.endsWith( "_international.xml", Qt::CaseInsensitive ) && galleryFile.open( QIODevice::ReadOnly ) )
      {
        QImage galleryIcon( QString( galleryFilePath ).replace( QRegExp( ".xml$" ), ".png" ) );
        QDomDocument doc;
        doc.setContent( &galleryFile );
        QDomElement mssGalleryEl = doc.firstChildElement( "MssGallery" );
        QDomElement galleryNameEl = mssGalleryEl.firstChildElement( QString( "Name_%1" ).arg( lang ) );
        if ( galleryNameEl.isNull() )
        {
          galleryNameEl = mssGalleryEl.firstChildElement( "Name_EN" );
        }
        QStandardItem *galleryItem = addItem( model->invisibleRootItem(), galleryNameEl.text(), galleryIcon );

        QDomNodeList sectionNodes = mssGalleryEl.elementsByTagName( "Section" );
        for ( int iSection = 0, nSections = sectionNodes.size(); iSection < nSections; ++iSection )
        {
          QDomElement sectionEl = sectionNodes.at( iSection ).toElement();
          QDomElement sectionNameEl = sectionEl.firstChildElement( QString( "Name_%1" ).arg( lang ) );
          if ( sectionNameEl.isNull() )
          {
            sectionNameEl = mssGalleryEl.firstChildElement( "Name_EN" );
          }
          QStandardItem *sectionItem = addItem( galleryItem, sectionNameEl.text() );

          QDomNodeList subSectionNodes = sectionEl.elementsByTagName( "SubSection" );
          for ( int iSubSection = 0, nSubSections = subSectionNodes.size(); iSubSection < nSubSections; ++iSubSection )
          {
            QDomElement subSectionEl = subSectionNodes.at( iSubSection ).toElement();
            QDomElement subSectionNameEl = subSectionEl.firstChildElement( QString( "Name_%1" ).arg( lang ) );
            if ( subSectionNameEl.isNull() )
            {
              subSectionNameEl = mssGalleryEl.firstChildElement( "Name_EN" );
            }
            QStandardItem *subSectionItem = addItem( sectionItem, subSectionNameEl.text() );

            QDomNodeList memberNodes = subSectionEl.elementsByTagName( "Member" );
            QStringList symbolXmls;
            for ( int iMember = 0, nMembers = memberNodes.size(); iMember < nMembers; ++iMember )
            {
              symbolXmls.append( memberNodes.at( iMember ).toElement().attribute( "MssStringXML" ) );
            }
            QList<KadasMilxSymbolDesc> symbolDescs;
            KadasMilxClient::getSymbolsMetadata( symbolXmls, symbolDescs );
            for ( const KadasMilxSymbolDesc &symbolDesc : symbolDescs )
            {
              if ( mLoaderAborted )
                return model;
              addItem( subSectionItem, symbolDesc.name, symbolDesc.icon, viewIconSize, true, symbolDesc.symbolXml, symbolDesc.militaryName, symbolDesc.minNumPoints, symbolDesc.hasVariablePoints );
            }
          }
        }
      }
    }
  }
  addItem( model->invisibleRootItem(), tr( "More Symbols..." ), QImage( ":/images/themes/default/mActionAdd.svg" ), viewIconSize, true, "<custom>", tr( "More Symbols..." ) );
  return model;
}

QStandardItem *KadasMilxLibrary::addItem( QStandardItem *parent, const QString &value, const QImage &image, const QSize &viewIconSize, bool isLeaf, const QString &symbolXml, const QString &symbolMilitaryName, int symbolPointCount, bool symbolHasVariablePoints )
{
  QIcon icon;
  QSize iconSize = isLeaf ? viewIconSize : !image.isNull() ? QSize( 32, 32 ) : QSize( 1, 32 );
  QImage iconImage( iconSize, QImage::Format_ARGB32 );
  iconImage.fill( Qt::transparent );
  if ( !image.isNull() )
  {
    double scale = qMin( 1., image.width() > image.height() ? iconImage.width() / double( image.width() ) : iconImage.height() / double( image.height() ) );
    QPainter painter( &iconImage );
    painter.setRenderHint( QPainter::SmoothPixmapTransform );
    painter.drawImage(
      QRectF( 0.5 * ( iconSize.width() - scale * image.width() ), 0.5 * ( iconSize.height() - scale * image.height() ), scale * image.width(), scale * image.height() ),
      image );
  }
  icon = QIcon( QPixmap::fromImage( iconImage ) );
  // Create category group item if necessary
  if ( !isLeaf )
  {
    // Don't create subgroups with same text as parent
    if ( parent->text() == value )
    {
      return parent;
    }
    QStandardItem *groupItem = 0;
    for ( int i = 0, n = parent->rowCount(); i < n; ++i )
    {
      if ( parent->child( i )->text() == value )
      {
        groupItem = parent->child( i );
        break;
      }
    }
    if ( !groupItem )
    {
      groupItem = new QStandardItem( value );
      groupItem->setDragEnabled( false );
      parent->setChild( parent->rowCount(), groupItem );
      groupItem->setIcon( icon );
    }
    return groupItem;
  }
  else
  {
    QStandardItem *item = new QStandardItem( QString( "%1" ).arg( symbolMilitaryName ) );
    parent->setChild( parent->rowCount(), item );
    item->setData( symbolXml, SymbolXmlRole );
    item->setData( symbolMilitaryName, SymbolMilitaryNameRole );
    item->setData( symbolPointCount, SymbolPointCountRole );
    item->setData( symbolHasVariablePoints, SymbolVariablePointsRole );
    item->setToolTip( item->text() );
    item->setIcon( icon );
    return item;
  }
}
