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

  // QgsAnnotationPictureItem::writeXml/readXml stores the raw mPath, with no
  // pathResolver translation. Pictures created via Kadas paths live in the
  // project archive's attachment dir (a temp folder with a random name) so
  // a literal save of the path makes the picture unrenderable on reload —
  // the renderer feeds the dead path into svg/imageCache, those return
  // nothing, and the image cache draws a "?" placeholder.
  //
  // Workaround: rewrite all picture paths that point inside the current
  // attachment dir to "attachment:<basename>" before save and resolve them
  // back to the (now extracted) real path right after load. The save-side
  // code also restores the live path after write so the in-memory state is
  // unchanged for the running session.

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

  // (id, item) pairs for all picture items on \a layer. Needed when the
  // caller has to address per-item layer customProperties (offset
  // side-channel keyed by item id).
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

  // Convert live attachment paths -> "attachment:..." identifiers.
  // Returns the previous paths so the caller can restore them after write.
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

  // After load, "attachment:..." identifiers must be resolved against the
  // newly-extracted archive directory before the renderer can read them.
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
  // QgsAnnotationItem::writeCommonProperties guards the offset write
  // with `if ( mOffsetFromCallout.isValid() )`. QSizeF::isValid() is
  // false for any negative dimension, so a centered-on-anchor offset
  // (-size/2, -size/2) — the canonical "no balloon visible" state used
  // by Kadas pictures — is silently dropped on save. Reload then
  // resets the offset to the default-invalid sentinel and the picture
  // jumps to the wrong place ("balloon back on the anchor").
  //
  // Workaround: write the offset (and its unit) into the layer's
  // customProperties under `kadas:picture-offset:<itemId>` and
  // `kadas:picture-offset-unit:<itemId>`. The layer XML always
  // round-trips customProperties, so the value survives.

  QString offsetKey( const QString &itemId )
  {
    return QStringLiteral( "kadas:picture-offset:" ) + itemId;
  }
  QString offsetUnitKey( const QString &itemId )
  {
    return QStringLiteral( "kadas:picture-offset-unit:" ) + itemId;
  }
  // Marker for "user explicitly turned the balloon off". Without this
  // marker, ensureBalloonsAfterLoad would helpfully re-install a
  // balloon on every reload and the user's choice would be lost.
  QString noCalloutKey( const QString &itemId )
  {
    return QStringLiteral( "kadas:picture-no-callout:" ) + itemId;
  }

  void writePictureNoCalloutFlags()
  {
    for ( QgsAnnotationLayer *layer : annotationLayers() )
    {
      for ( const auto &pair : picturesInWithIds( layer ) )
      {
        const QString id = pair.first;
        const QgsAnnotationPictureItem *pic = pair.second;
        if ( !pic->callout() )
          layer->setCustomProperty( noCalloutKey( id ), true );
        else
          layer->removeCustomProperty( noCalloutKey( id ) );
      }
    }
  }

  void writePictureOffsetsToCustomProperties()
  {
    for ( QgsAnnotationLayer *layer : annotationLayers() )
    {
      for ( const auto &pair : picturesInWithIds( layer ) )
      {
        const QString id = pair.first;
        const QgsAnnotationPictureItem *pic = pair.second;
        // Only write when something non-default exists. An invalid
        // size (default-constructed QSizeF) means "QGIS-default", we
        // don't need to side-channel that.
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

  // Walk every picture and run ensureBalloon. Handles two migration
  // cases on project load:
  //  - Project saved by an older Kadas (or by vanilla QGIS) without a
  //    callout installed — install the canonical balloon.
  //  - Project where the saved offset was the QGIS default (invalid),
  //    e.g. saved before the side-channel above was wired up — give
  //    the picture a centered-on-anchor offset so it renders sensibly.
  void ensureBalloonsAfterLoad()
  {
    for ( QgsAnnotationLayer *layer : annotationLayers() )
    {
      for ( const auto &pair : picturesInWithIds( layer ) )
      {
        const QString id = pair.first;
        QgsAnnotationPictureItem *pic = pair.second;
        // Respect the user's explicit "Show callout" = off choice.
        // Without this guard, every reload would re-install a balloon
        // and the toggle would feel one-shot.
        if ( layer->customProperty( noCalloutKey( id ), false ).toBool() )
          continue;
        KadasPictureAnnotationController::ensureBalloon( pic );
      }
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
  // Persist the offsetFromCallout via the layer customProperty side
  // channel because QGIS drops negative offsets at write time. See the
  // long comment above writePictureOffsetsToCustomProperties.
  writePictureOffsetsToCustomProperties();
  writePictureNoCalloutFlags();
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
  // Defensive: a project written by an older Kadas session might still
  // contain shadow items if the save-side strip didn't run. Drop them now
  // so they don't appear as duplicates next to the masters.
  stripAfterSave();
  // Re-point picture items at the freshly-extracted attachment files in
  // the archive's temp directory; the renderer reads mPath verbatim.
  resolveAttachmentPathsAfterLoad();
  // Restore offset side-channel before ensureBalloon runs so a saved
  // negative offset is honored instead of being clobbered by the
  // -size/2 default.
  readPictureOffsetsFromCustomProperties();
  // Migrate calloutless / partially-configured pictures to the canonical
  // Kadas balloon look so editing UX is uniform regardless of how the
  // picture was originally created.
  ensureBalloonsAfterLoad();
}
