/***************************************************************************
    kadascatalogbrowser.h
    ---------------------
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

#ifndef KADASCATALOGBROWSER_H
#define KADASCATALOGBROWSER_H

#include <QStandardItemModel>
#include <QWidget>

#include <qgis/qgsmimedatautils.h>
#include <qgis/qgssettingsentryimpl.h>

#include "kadas/gui/kadas_gui.h"
#include "kadas/core/kadassettingstree.h"


class KadasCatalogProvider;
class QAbstractItemModel;
class QgsFilterLineEdit;
class QTimer;
class QToolButton;
class QTreeView;


class KADAS_GUI_EXPORT KadasCatalogBrowser : public QWidget
{
    Q_OBJECT
  public:
    static const inline QgsSettingsEntryBool *sSettingLoadArcgiseatureserverLayersAsRaster
      = new QgsSettingsEntryBool( QStringLiteral( "load-arcgisfeatureserver-layers-as-raster" ), KadasSettingsTree::sTreePortal, true ) SIP_SKIP;

    KadasCatalogBrowser( QWidget *parent = 0 );
    void addProvider( KadasCatalogProvider *provider ) { mProviders.append( provider ); }
    QStandardItem *addItem( QStandardItem *parent, QString text, int sortIndex, bool isLeaf = false, QMimeData *mimeData = nullptr );

    //! Button which reloads the catalog, shown next to the filter field.
    QToolButton *refreshButton() const { return mRefreshButton; }

    //! TRUE while selecting an entry previews it on the map.
    bool autoPreviewEnabled() const;

  public slots:
    void reload();

  signals:
    void layerSelected( const QgsMimeDataUtils::Uri &uri, const QString &metadataUrl, const QVariantList &sublayers );

    /**
     * Emitted when \a uri should be shown on the map without being added,
     * shortly after the user selected it with auto preview enabled.
     */
    void previewRequested( const QgsMimeDataUtils::Uri &uri, const QVariantList &sublayers );

    //! Emitted when nothing should be previewed any longer.
    void previewCleared();

  protected:
    /**
     * The preview follows the browser: it only makes sense while the entry
     * driving it can be seen. These cover the catalog column being hidden, the
     * layers panel being collapsed and the window being closed alike, since Qt
     * passes hide and show on to the children of the widget being toggled.
     */
    void showEvent( QShowEvent *event ) override;
    void hideEvent( QHideEvent *event ) override;

  private:
    class CatalogModel;
    class CatalogItem;
    class TreeFilterProxyModel;

    /**
     * Sets \a model on the tree view and reconnects the selection handling,
     * which the view recreates with every model change.
     */
    void setTreeModel( QAbstractItemModel *model );

    QgsFilterLineEdit *mFilterLineEdit;
    QToolButton *mRefreshButton;
    QToolButton *mPreviewButton;
    QTreeView *mTreeView;
    CatalogModel *mCatalogModel;
    QStandardItemModel *mLoadingModel;
    QStandardItemModel *mOfflineModel;
    TreeFilterProxyModel *mFilterProxyModel;
    QList<KadasCatalogProvider *> mProviders;
    int mFinishedProviders;

    /**
     * Debounce for the preview: browsing with the arrow keys would otherwise
     * fire a request per row passed over.
     */
    QTimer *mPreviewTimer;

  private slots:
    void filterChanged( const QString &text );
    void itemDoubleClicked( const QModelIndex &index );
    void providerFinished();
    //! Emits previewRequested()/previewCleared() for the current selection.
    void updatePreview();
};

#endif // KADASCATALOGBROWSER_H
