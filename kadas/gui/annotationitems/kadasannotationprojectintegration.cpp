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

#include <qgis/qgsannotationitem.h>
#include <qgis/qgsannotationlayer.h>
#include <qgis/qgsannotationpictureitem.h>
#include <qgis/qgsproject.h>

#include "kadas/gui/annotationitems/kadasannotationlayerhelpers.h"
#include "kadas/gui/annotationitems/kadasannotationprojectintegration.h"


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
}
