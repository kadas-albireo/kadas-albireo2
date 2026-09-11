/***************************************************************************
    kadasattachmentutils.cpp
    ------------------------
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

#include <QFileInfo>
#include <QHash>
#include <QImage>
#include <QList>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextFragment>
#include <QUrl>
#include <QUrlQuery>
#include <algorithm>
#include <memory>

#include <qgis/qgsproject.h>

#include "kadas/gui/kadasattachmentutils.h"


bool KadasAttachmentUtils::decodeDataUrl( const QString &url, QImage &image, QString &suffix )
{
  static const QLatin1String sPrefix( "data:image/" );
  static const QLatin1String sBase64Marker( ";base64" );

  if ( !url.startsWith( sPrefix, Qt::CaseInsensitive ) )
    return false;
  const int comma = url.indexOf( QLatin1Char( ',' ) );
  if ( comma < 0 )
    return false;
  const QString header = url.left( comma );
  if ( !header.endsWith( sBase64Marker, Qt::CaseInsensitive ) )
    return false;
  const QByteArray payload = QByteArray::fromBase64( url.mid( comma + 1 ).toLatin1() );
  if ( payload.isEmpty() || !image.loadFromData( payload ) )
    return false;
  // The header's middle section is a throwaway name QGIS invents, but it does
  // carry the source format: "data:image/1234.PNG;base64" -> "png".
  suffix = QFileInfo( header.mid( sPrefix.size(), header.size() - sPrefix.size() - sBase64Marker.size() ) ).suffix().toLower();
  if ( suffix.isEmpty() )
    suffix = QStringLiteral( "png" );
  return true;
}

bool KadasAttachmentUtils::isIdentifier( const QString &url )
{
  return url.startsWith( sIdentifierPrefix );
}

QString KadasAttachmentUtils::canonicalIdentifier( const QString &identifier )
{
  // QgsProject resolves only "attachment:///name"; Kadas 2.x also wrote a bare
  // "attachment:name".
  if ( isIdentifier( identifier ) && !identifier.startsWith( sCanonicalPrefix ) )
    return sCanonicalPrefix + identifier.mid( sIdentifierPrefix.size() );
  return identifier;
}

QString KadasAttachmentUtils::resolve( const QString &identifier )
{
  if ( identifier.isEmpty() )
    return QString();
  // Any "?w=&h=" display size is not part of the file name.
  const QString id = canonicalIdentifier( identifier.section( QLatin1Char( '?' ), 0, 0 ) );
  const QString path = QgsProject::instance()->resolveAttachmentIdentifier( id );
  return QFileInfo::exists( path ) ? path : QString();
}

QString KadasAttachmentUtils::materializeInlineImages( const QString &html, int maxStoredSize, int maxDisplaySize )
{
  // Parsing the markup costs far more than looking for the one thing that would
  // make it worth parsing, and a description with no inline image is the norm.
  if ( !html.contains( QLatin1String( "data:image/" ), Qt::CaseInsensitive ) )
    return html;

  QTextDocument doc;
  doc.setHtml( html );

  struct Rewrite
  {
      int position = 0;
      int length = 0;
      QTextImageFormat format;
  };
  QList<Rewrite> rewrites;

  for ( QTextBlock block = doc.begin(); block.isValid(); block = block.next() )
  {
    for ( QTextBlock::iterator it = block.begin(); !it.atEnd(); ++it )
    {
      const QTextFragment fragment = it.fragment();
      if ( !fragment.isValid() || !fragment.charFormat().isImageFormat() )
        continue;
      QTextImageFormat format = fragment.charFormat().toImageFormat();
      QImage image;
      QString suffix;
      if ( !decodeDataUrl( format.name(), image, suffix ) )
        continue;

      const QString identifier = attachImage( image, suffix, maxStoredSize );
      if ( identifier.isEmpty() )
        continue;
      format.setName( identifier );

      // The file keeps its resolution so it opens usefully at full size; the
      // markup only says how big to draw it.
      QSize displaySize = image.size();
      if ( std::max( displaySize.width(), displaySize.height() ) > maxStoredSize )
        displaySize.scale( maxStoredSize, maxStoredSize, Qt::KeepAspectRatio );
      if ( std::max( displaySize.width(), displaySize.height() ) > maxDisplaySize )
        displaySize.scale( maxDisplaySize, maxDisplaySize, Qt::KeepAspectRatio );
      displaySize = displaySize.expandedTo( QSize( 1, 1 ) );
      format.setWidth( displaySize.width() );
      format.setHeight( displaySize.height() );
      for ( int offset = 0; offset < fragment.length(); ++offset )
        rewrites.append( { fragment.position() + offset, 1, format } );
    }
  }

  if ( rewrites.isEmpty() )
    return html;

  // Back to front, so replacing one image cannot shift the position of the next.
  QTextCursor cursor( &doc );
  for ( int i = rewrites.size() - 1; i >= 0; --i )
  {
    cursor.setPosition( rewrites.at( i ).position );
    cursor.setPosition( rewrites.at( i ).position + rewrites.at( i ).length, QTextCursor::KeepAnchor );
    cursor.insertImage( rewrites.at( i ).format );
  }
  return doc.toHtml();
}

QString KadasAttachmentUtils::attachImage( const QImage &image, const QString &suffix, int maxStoredSize )
{
  if ( image.isNull() )
    return QString();
  QImage stored = image;
  if ( std::max( stored.width(), stored.height() ) > maxStoredSize )
    stored = stored.scaled( maxStoredSize, maxStoredSize, Qt::KeepAspectRatio, Qt::SmoothTransformation );

  const QString file = QgsProject::instance()->createAttachedFile( QStringLiteral( "annotation_image.%1" ).arg( suffix.isEmpty() ? QStringLiteral( "png" ) : suffix ) );
  if ( file.isEmpty() || !stored.save( file ) )
    return QString();
  return QgsProject::instance()->attachmentIdentifier( file );
}

bool KadasAttachmentUtils::clampImageDisplaySize( QTextDocument *document, int maxDisplaySize )
{
  if ( !document )
    return false;

  struct Rewrite
  {
      int position = 0;
      int length = 0;
      QTextImageFormat format;
  };
  QList<Rewrite> rewrites;

  for ( QTextBlock block = document->begin(); block.isValid(); block = block.next() )
  {
    for ( QTextBlock::iterator it = block.begin(); !it.atEnd(); ++it )
    {
      const QTextFragment fragment = it.fragment();
      if ( !fragment.isValid() || !fragment.charFormat().isImageFormat() )
        continue;
      QTextImageFormat format = fragment.charFormat().toImageFormat();
      double width = format.width();
      double height = format.height();
      if ( width <= 0 || height <= 0 )
      {
        // No explicit size on the fragment: fall back to the image's own.
        const QImage image = qvariant_cast<QImage>( document->resource( QTextDocument::ImageResource, QUrl( format.name() ) ) );
        if ( image.isNull() )
          continue;
        width = image.width();
        height = image.height();
      }
      const double longest = std::max( width, height );
      if ( longest <= maxDisplaySize )
        continue;
      const double factor = maxDisplaySize / longest;
      format.setWidth( std::max( 1, qRound( width * factor ) ) );
      format.setHeight( std::max( 1, qRound( height * factor ) ) );
      for ( int offset = 0; offset < fragment.length(); ++offset )
        rewrites.append( { fragment.position() + offset, 1, format } );
    }
  }

  if ( rewrites.isEmpty() )
    return false;

  QTextCursor cursor( document );
  // Fold the resize into the edit that inserted the image, so a single undo
  // takes the image back out rather than first restoring its original size.
  cursor.joinPreviousEditBlock();
  for ( int i = rewrites.size() - 1; i >= 0; --i )
  {
    cursor.setPosition( rewrites.at( i ).position );
    cursor.setPosition( rewrites.at( i ).position + rewrites.at( i ).length, QTextCursor::KeepAnchor );
    cursor.insertImage( rewrites.at( i ).format );
  }
  cursor.endEditBlock();
  return true;
}

void KadasAttachmentUtils::installResourceProvider( QTextDocument *document )
{
  if ( !document )
    return;
  // Installing a provider bypasses QTextDocument's own resource cache, and Qt
  // asks for each image several times over a layout and paint. Without a cache
  // of our own, every one of those re-reads and re-decodes the file - a
  // full-size photo decoded a dozen times per hover.
  auto cache = std::make_shared<QHash<QString, QImage>>();
  document->setResourceProvider( [cache]( const QUrl &url ) -> QVariant {
    if ( url.scheme() != sScheme )
      return QVariant();
    // Keyed by the whole URL, so the same file asked for at two display sizes
    // keeps two entries rather than one of them winning.
    const QString key = url.toString();
    const auto cached = cache->constFind( key );
    if ( cached != cache->constEnd() )
      return *cached;

    // QUrl::path() drops the scheme and its slashes, and how many of those
    // there were depends on the spelling: Kadas 2.x's bare "attachment:name"
    // leaves "name" where "attachment:///name" leaves "/name". Put back the one
    // spelling QgsProject resolves rather than whichever was written.
    QString path = url.path();
    while ( path.startsWith( QLatin1Char( '/' ) ) )
      path.remove( 0, 1 );
    const QString file = resolve( sCanonicalPrefix + path );
    if ( file.isEmpty() )
      return QVariant();
    QImage image( file );
    if ( image.isNull() )
      return QVariant();
    const QUrlQuery query( url.query() );
    const int width = query.queryItemValue( QStringLiteral( "w" ) ).toInt();
    const int height = query.queryItemValue( QStringLiteral( "h" ) ).toInt();
    if ( width > 0 && height > 0 )
      image = image.scaled( width, height, Qt::IgnoreAspectRatio, Qt::SmoothTransformation );

    // A single document outlives many items — the map tooltip is reused for
    // every annotation hovered — so the cache must not grow along with them.
    // A miss costs one decode, which is what this saves a dozen of.
    if ( cache->size() >= sMaxCachedImages )
      cache->clear();
    cache->insert( key, image );
    return image;
  } );
}
