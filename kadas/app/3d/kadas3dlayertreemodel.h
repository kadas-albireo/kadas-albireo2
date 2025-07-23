#ifndef KADAS3DLAYERTREEMODEL_H
#define KADAS3DLAYERTREEMODEL_H

#include <QSortFilterProxyModel>
#include <QItemDelegate>

#include <qgis/qgslayertreemodel.h>
#include <qgis/qgsmapthemecollection.h>


class QgsProject;
class Qgs3DMapCanvas;


class Kadas3DLayerTreeModel : public QSortFilterProxyModel
{
    Q_OBJECT

  public:
    static const QString TERRAIN_MAP_THEME;

    Kadas3DLayerTreeModel( Qgs3DMapCanvas *mapCanvas = nullptr );

    int columnCount( const QModelIndex &parent ) const override;
    QVariant headerData( int section, Qt::Orientation orientation, int role ) const override;
    Qt::ItemFlags flags( const QModelIndex &idx ) const override;
    QModelIndex index( int row, int column, const QModelIndex &parent = QModelIndex() ) const override;
    QModelIndex parent( const QModelIndex &child ) const override;
    QModelIndex sibling( int row, int column, const QModelIndex &idx ) const override;
    QVariant data( const QModelIndex &index, int role ) const override;
    bool setData( const QModelIndex &index, const QVariant &value, int role ) override;

    QgsLayerTreeModel *layerTreeModel() const;
    void setLayerTreeModel( QgsLayerTreeModel *layerTreeModel );

    QgsMapLayer *mapLayer( const QModelIndex &idx ) const;

  public slots:
    void setFilterText( const QString &filterText = QString() );

  protected:
    bool filterAcceptsRow( int sourceRow, const QModelIndex &sourceParent ) const override;

  private slots:
    //void onSnappingSettingsChanged();

  private:
    bool nodeShown( QgsLayerTreeNode *node ) const;
    void createThemeFromCurrentState( QgsLayerTreeGroup *parent, QgsMapThemeCollection::MapThemeRecord &rec );
    void visibleLayers( QgsLayerTreeGroup *parent, QSet<QgsMapLayer *> &layers );

    QSet<QgsMapLayer *> mShownLayers;
    QgsLayerTreeGroup *mRoot = nullptr;
    QString mFilterText;
    QgsLayerTreeModel *mLayerTreeModel = nullptr;
    Qgs3DMapCanvas *mMapCanvas = nullptr;
};


#endif // KADAS3DLAYERTREEMODEL_H
