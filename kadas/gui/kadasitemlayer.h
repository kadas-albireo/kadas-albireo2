/***************************************************************************
    kadasitemlayer.h
    ----------------
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

#ifndef KADASITEMLAYER_H
#define KADASITEMLAYER_H

#include <qgis/qgspluginlayer.h>
#include <qgis/qgspluginlayerregistry.h>

#include <kadas/core/kadaspluginlayer.h>
#include <kadas/gui/kadas_gui.h>

class QMenu;
class QuaZip;
class KadasMapItem;

#ifdef SIP_RUN
//
// copied from PyQt4 QMap<int, TYPE> and adapted to unsigned
//
% MappedType QMap<unsigned, KadasMapItem *>
{
  % TypeHeaderCode
#include <QMap>
  % End

  % ConvertFromTypeCode
  // Create the dictionary.
  PyObject *d = PyDict_New();

  if ( !d )
    return NULL;

  // Set the dictionary elements.
  QMap<unsigned, KadasMapItem *>::const_iterator i = sipCpp->constBegin();

  while ( i != sipCpp->constEnd() )
  {
    KadasMapItem *t2 = i.value();

    PyObject *t1obj = PyLong_FromLong( i.key() );
    PyObject *t2obj = sipConvertFromType( t2, sipType_KadasMapItem, sipTransferObj );

    if ( t1obj == NULL || t2obj == NULL || PyDict_SetItem( d, t1obj, t2obj ) < 0 )
    {
      Py_DECREF( d );

      if ( t1obj )
        Py_DECREF( t1obj );

      if ( t2obj )
        Py_DECREF( t2obj );
      else
        delete t2;

      return NULL;
    }

    Py_DECREF( t1obj );
    Py_DECREF( t2obj );

    ++i;
  }

  return d;
  % End

  % ConvertToTypeCode
  PyObject * t1obj, *t2obj;
  Py_ssize_t i = 0;

  // Check the type if that is all that is required.
  if ( sipIsErr == NULL )
  {
    if ( !PyDict_Check( sipPy ) )
      return 0;

    while ( PyDict_Next( sipPy, &i, &t1obj, &t2obj ) )
    {
      if ( !sipCanConvertToType( t2obj, sipType_KadasMapItem, SIP_NOT_NONE ) )
        return 0;
    }

    return 1;
  }

  QMap<unsigned, KadasMapItem *> *qm = new QMap<unsigned, KadasMapItem *>;

  while ( PyDict_Next( sipPy, &i, &t1obj, &t2obj ) )
  {
    int state2;

    unsigned t1 = PyLong_AsLong( t1obj );
    KadasMapItem *t2 = reinterpret_cast<KadasMapItem *>( sipConvertToType( t2obj, sipType_KadasMapItem, sipTransferObj, SIP_NOT_NONE, &state2, sipIsErr ) );

    if ( *sipIsErr )
    {
      sipReleaseType( t2, sipType_KadasMapItem, state2 );

      delete qm;
      return 0;
    }

    qm->insert( t1, t2 );

    sipReleaseType( t2, sipType_KadasMapItem, state2 );
  }

  *sipCppPtr = qm;

  return sipGetState( sipTransferObj );
  % End
};
#endif

class KADAS_GUI_EXPORT KadasItemLayer : public KadasPluginLayer
{
    Q_OBJECT
  public:
    typedef unsigned ItemId;
    static constexpr ItemId ITEM_ID_NULL = 0;

    static QString layerType() { return "KadasItemLayer"; }
    KadasItemLayer( const QString &name, const QgsCoordinateReferenceSystem &crs );
    ~KadasItemLayer();
    QString layerTypeKey() const override { return layerType(); };

    void addItem( KadasMapItem *item SIP_TRANSFER );
    KadasMapItem *takeItem( const ItemId &itemId ) SIP_TRANSFER;
    const QMap<KadasItemLayer::ItemId, KadasMapItem *> &items() const { return mItems; }

    KadasItemLayer *clone() const override SIP_FACTORY;
    QgsMapLayerRenderer *createMapRenderer( QgsRenderContext &rendererContext ) override;
    QgsRectangle extent() const override;
    bool readXml( const QDomNode &layer_node, QgsReadWriteContext &context ) override;
    bool writeXml( QDomNode &layer_node, QDomDocument &document, const QgsReadWriteContext &context ) const override;
    virtual KadasItemLayer::ItemId pickItem( const QgsRectangle &pickRect, const QgsMapSettings &mapSettings ) const;
    KadasItemLayer::ItemId pickItem( const QgsPointXY &mapPos, const QgsMapSettings &mapSettings ) const;

#ifndef SIP_RUN
    // TODO: SIP
    QPair<QgsPointXY, double> snapToVertex( const QgsPointXY &pos, const QgsMapSettings &settings, double tolPixels ) const;
#endif

#ifndef SIP_RUN
    virtual QString asKml( const QgsRenderContext &context, QuaZip *kmzZip = nullptr, const QgsRectangle &exportRect = QgsRectangle() ) const;
#endif

    void setSymbolScale( double scale );
    double symbolScale() const { return mSymbolScale; }

  signals:
    void itemAdded( KadasItemLayer::ItemId itemId );
    void itemRemoved( KadasItemLayer::ItemId itemId );

  protected:
    KadasItemLayer( const QString &name, const QgsCoordinateReferenceSystem &crs, const QString &layerType );
    class Renderer;

    QMap<ItemId, KadasMapItem *> mItems;
    QList<ItemId> mItemOrder;
    QMap<ItemId, QgsRectangle> mItemBounds;
    ItemId mIdCounter = 0;
    QVector<ItemId> mFreeIds;
    double mSymbolScale = 1.0;
};

class KADAS_GUI_EXPORT KadasItemLayerType : public KadasPluginLayerType
{
  public:
    KadasItemLayerType()
      : KadasPluginLayerType( KadasItemLayer::layerType() ) {}
    QgsPluginLayer *createLayer() override SIP_FACTORY { return new KadasItemLayer( "Items", QgsCoordinateReferenceSystem( "EPSG:3857" ) ); }
    QgsPluginLayer *createLayer( const QString &uri ) override SIP_FACTORY { return new KadasItemLayer( "Items", QgsCoordinateReferenceSystem( "EPSG:3857" ) ); }
    void addLayerTreeMenuActions( QMenu *menu, QgsPluginLayer *layer ) const;
};

class KADAS_GUI_EXPORT KadasItemLayerRegistry : public QObject
{
    Q_OBJECT
  public:
    enum StandardLayer { RedliningLayer, SymbolsLayer, PicturesLayer, PinsLayer, RoutesLayer };
    static KadasItemLayer *getOrCreateItemLayer( StandardLayer layer );
    static KadasItemLayer *getOrCreateItemLayer( const QString &layerName );
    static const QMap<KadasItemLayerRegistry::StandardLayer, QString> &standardLayerNames();

  private:
    static KadasItemLayer *getItemLayer( const QString &layerName );
    static QMap<QString, QString> &layerIdMap();
};

#endif // KADASITEMLAYER_H
