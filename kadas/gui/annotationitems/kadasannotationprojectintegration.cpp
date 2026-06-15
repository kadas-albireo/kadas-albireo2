/***************************************************************************
    kadasannotationprojectintegration.cpp
    -------------------------------------
    copyright            : (C) 2026 by Denis Rouzaud
    email                : denis at opengis dot ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <QDomDocument>
#include <QDir>
#include <QFileInfo>

#include <qgis/qgis.h>
#include <qgis/qgsannotationitem.h>
#include <qgis/qgsannotationlayer.h>
#include <qgis/qgsannotationpictureitem.h>
#include <qgis/qgsproject.h>
#include <qgis/qgssymbollayerutils.h>
#include <qgis/qgsunittypes.h>

#include "kadas/gui/annotationitems/kadasannotationlayerhelpers.h"
#include "kadas/gui/annotationitems/kadasannotationprojectintegration.h"
#include "kadas/gui/annotationitems/kadaspictureannotationcontroller.h"


namespace
{
  QList<QgsAnnotationLayer *> annotationLayers()
  {
    QList<QgsAnnotationLayer *> result;
    const QMap<QString, QgsMapLayer *> layers = QgsProject::instance()->mapLayers();
    for ( QgsMapLayer *layer : layers )
    {
      if ( auto *al = qobject_cast<QgsAnnotationLayer *>( layer ) )
        result.append( al );
    }
    // Also include the project main annotation layer.
    if ( auto *main = QgsProject::instance()->mainAnnotationLayer() )
    {
      if ( !result.contains( main ) )
        result.append( main );
    }
    return result;
  }

  // QgsAnnotationPictureItem::writeXml stores the raw path verbatim, so attachment-dir paths break on reload; rewrite to "attachment:<name>" before save and resolve after load.

  QList<QgsAnnotationPictureItem *> picturesIn( QgsAnnotationLayer *layer )
  {
    QList<QgsAnnotationPictureItem *> result;
    const QMap<QString, QgsAnnotationItem *> items = layer->items();
    for ( QgsAnnotationItem *item : items )
    {
      if ( auto *pic = dynamic_cast<QgsAnnotationPictureItem *>( item ) )
        result.append( pic );
    }
    return result;
  }

  // (id, item) pairs for all picture items on \a layer.
  QList<QPair<QString, QgsAnnotationPictureItem *>> picturesInWithIds( QgsAnnotationLayer *layer )
  {
    QList<QPair<QString, QgsAnnotationPictureItem *>> result;
    const QMap<QString, QgsAnnotationItem *> items = layer->items();
    for ( auto it = items.constBegin(); it != items.constEnd(); ++it )
    {
      if ( auto *pic = dynamic_cast<QgsAnnotationPictureItem *>( it.value() ) )
        result.append( qMakePair( it.key(), pic ) );
    }
    return result;
  }

  bool isAttachmentIdentifier( const QString &path )
  {
    return path.startsWith( QLatin1String( "attachment:" ) );
  }

  QString toAttachmentIdentifier( const QString &absolutePath )
  {
    // QgsProject::attachmentIdentifier returns "attachment:///filename".
    return QgsProject::instance()->attachmentIdentifier( absolutePath );
  }

  bool isUnderProjectAttachmentDir( const QString &absolutePath )
  {
    if ( absolutePath.isEmpty() )
      return false;
    const QStringList attached = QgsProject::instance()->attachedFiles();
    return attached.contains( absolutePath );
  }

  // Convert live attachment paths to "attachment:..." identifiers; returns previous paths for restore.
  QHash<QgsAnnotationPictureItem *, QString> rewritePathsForSave()
  {
    QHash<QgsAnnotationPictureItem *, QString> previous;
    for ( QgsAnnotationLayer *layer : annotationLayers() )
    {
      for ( QgsAnnotationPictureItem *pic : picturesIn( layer ) )
      {
        const QString p = pic->path();
        if ( isAttachmentIdentifier( p ) )
          continue;
        if ( !isUnderProjectAttachmentDir( p ) )
          continue;
        previous.insert( pic, p );
        pic->setPath( pic->format(), toAttachmentIdentifier( p ) );
      }
    }
    return previous;
  }

  void restorePathsAfterSave( const QHash<QgsAnnotationPictureItem *, QString> &previous )
  {
    for ( auto it = previous.constBegin(); it != previous.constEnd(); ++it )
      it.key()->setPath( it.key()->format(), it.value() );
  }

  // After load, resolve "attachment:..." identifiers against the extracted archive dir.
  void resolveAttachmentPathsAfterLoad()
  {
    for ( QgsAnnotationLayer *layer : annotationLayers() )
    {
      for ( QgsAnnotationPictureItem *pic : picturesIn( layer ) )
      {
        const QString p = pic->path();
        if ( !isAttachmentIdentifier( p ) )
          continue;
        const QString resolved = QgsProject::instance()->resolveAttachmentIdentifier( p );
        if ( !resolved.isEmpty() && QFileInfo::exists( resolved ) )
          pic->setPath( pic->format(), resolved );
      }
    }
  }

  // ---- offsetFromCallout side-channel ----
  //
  // QgsAnnotationItem::writeCommonProperties drops negative offsetFromCallout (QSizeF::isValid() false), so persist it via layer customProperties instead.

  QString offsetKey( const QString &itemId )
  {
    return QStringLiteral( "kadas:picture-offset:" ) + itemId;
  }
  QString offsetUnitKey( const QString &itemId )
  {
    return QStringLiteral( "kadas:picture-offset-unit:" ) + itemId;
  }

  void writePictureOffsetsToCustomProperties()
  {
    for ( QgsAnnotationLayer *layer : annotationLayers() )
    {
      for ( const auto &pair : picturesInWithIds( layer ) )
      {
        const QString id = pair.first;
        const QgsAnnotationPictureItem *pic = pair.second;
        // Skip the default (zero) offset; only side-channel non-default values.
        const QSizeF off = pic->offsetFromCallout();
        if ( off.width() == 0 && off.height() == 0 )
        {
          layer->removeCustomProperty( offsetKey( id ) );
          layer->removeCustomProperty( offsetUnitKey( id ) );
          continue;
        }
        layer->setCustomProperty( offsetKey( id ), QgsSymbolLayerUtils::encodeSize( off ) );
        layer->setCustomProperty( offsetUnitKey( id ), QgsUnitTypes::encodeUnit( pic->offsetFromCalloutUnit() ) );
      }
    }
  }

  void readPictureOffsetsFromCustomProperties()
  {
    for ( QgsAnnotationLayer *layer : annotationLayers() )
    {
      for ( const auto &pair : picturesInWithIds( layer ) )
      {
        const QString id = pair.first;
        QgsAnnotationPictureItem *pic = pair.second;
        const QString sizeStr = layer->customProperty( offsetKey( id ) ).toString();
        if ( sizeStr.isEmpty() )
          continue;
        const QSizeF off = QgsSymbolLayerUtils::decodeSize( sizeStr );
        bool ok = false;
        Qgis::RenderUnit unit = QgsUnitTypes::decodeRenderUnit( layer->customProperty( offsetUnitKey( id ) ).toString(), &ok );
        if ( !ok )
          unit = pic->fixedSizeUnit();
        pic->setOffsetFromCallout( off );
        pic->setOffsetFromCalloutUnit( unit );
      }
    }
  }

  // Run ensureBalloon on every picture to migrate calloutless/default-offset pictures.
  void ensureBalloonsAfterLoad()
  {
    for ( QgsAnnotationLayer *layer : annotationLayers() )
    {
      for ( QgsAnnotationPictureItem *pic : picturesIn( layer ) )
        KadasPictureAnnotationController::ensureBalloon( pic );
    }
  }
} // namespace


KadasAnnotationProjectIntegration::KadasAnnotationProjectIntegration( QObject *parent )
  : QObject( parent )
{
  connect( QgsProject::instance(), &QgsProject::readProject, this, &KadasAnnotationProjectIntegration::onProjectRead );
}

void KadasAnnotationProjectIntegration::prepareForSave()
{
  for ( QgsAnnotationLayer *layer : annotationLayers() )
    KadasAnnotationLayerHelpers::prepareLayerForSave( layer );
  mPicturePathsBeforeSave = rewritePathsForSave();
  writePictureOffsetsToCustomProperties();
}

void KadasAnnotationProjectIntegration::stripAfterSave()
{
  restorePathsAfterSave( mPicturePathsBeforeSave );
  mPicturePathsBeforeSave.clear();
  for ( QgsAnnotationLayer *layer : annotationLayers() )
    KadasAnnotationLayerHelpers::stripShadowsFromLayer( layer );
}

void KadasAnnotationProjectIntegration::onProjectRead( const QDomDocument & )
{
  // Strip leftover shadow items from projects saved by older sessions.
  stripAfterSave();
  // Re-point picture items at the freshly-extracted attachment files.
  resolveAttachmentPathsAfterLoad();
  // Restore offset side-channel before ensureBalloon runs.
  readPictureOffsetsFromCustomProperties();
  // Migrate calloutless pictures to the canonical balloon look.
  ensureBalloonsAfterLoad();
}
