/***************************************************************************
    kadasattachmentutils.h
    ----------------------
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

#ifndef KADASATTACHMENTUTILS_H
#define KADASATTACHMENTUTILS_H

#include <QLatin1StringView>
#include <QList>
#include <QString>
#include <QTextFormat>

#include "kadas/gui/kadas_gui.h"

class QImage;
class QTextDocument;

/**
 * \ingroup gui
 * \brief Images kept in the project archive and referenced from rich text as
 *        "attachment:///name" URLs.
 *
 * QgsRichTextEditor embeds an inserted image inline, as a base64 data: URL at
 * the file's original resolution. materializeInlineImages() moves those into the
 * project archive instead, so the stored markup stays small and the file behind
 * an image can still be opened at its full size.
 */
class KADAS_GUI_EXPORT KadasAttachmentUtils
{
  public:
    //! TRUE if \a url references a project attachment, in any spelling Kadas has written.
    static bool isIdentifier( const QString &url );

    //! \a identifier in the canonical "attachment:///name" spelling, the only one QgsProject resolves.
    static QString canonicalIdentifier( const QString &identifier );

    //! Absolute path \a identifier points at, or an empty string when no such file exists.
    static QString resolve( const QString &identifier );

    /**
     * Rewrites every inline data: image in \a html into a project attachment and
     * returns the adjusted markup. Images that are already attachments, and
     * markup with no inline images at all, are returned untouched.
     *
     * The stored file is capped at \a maxStoredSize pixels on its longest edge,
     * so a phone photo cannot bloat the project, while the markup carries a
     * display size of at most \a maxDisplaySize pixels on its longest edge — the two are
     * deliberately different, so an image shown small in a tooltip still opens
     * usefully large.
     */
    static QString materializeInlineImages( const QString &html, int maxStoredSize = 1920, int maxDisplaySize = 280 );

    /**
     * Stores \a image in the project archive as a \a suffix file and returns its
     * attachment identifier, or an empty string if it could not be written.
     *
     * Capped at \a maxStoredSize pixels on its longest edge, so that a phone
     * photo cannot bloat the project while still being worth opening full size.
     */
    static QString attachImage( const QImage &image, const QString &suffix, int maxStoredSize = 1920 );

    /**
     * Clamps every image in \a document to at most \a maxDisplaySize pixels on
     * its longest edge, preserving aspect ratio and leaving the file behind it
     * untouched. Returns TRUE if anything changed.
     *
     * Bounding the longest edge rather than the width is what keeps a portrait
     * photo inside the tooltip it will be shown in, which does not grow.
     *
     * QgsRichTextEditor inserts an image at its full pixel size, so a photo
     * arrives in the editor many times wider than the field it will be shown in.
     */
    static bool clampImageDisplaySize( QTextDocument *document, int maxDisplaySize = 280 );

    /**
     * Teaches \a document to resolve "attachment:///name" image URLs against the
     * current project, which it has no way to do on its own. Inline data: images
     * keep working: Qt decodes those itself and never consults the provider.
     *
     * A "?w=&h=" query scales the image, the spelling Kadas 2.x stored its
     * display sizes in.
     *
     * Decoded images are cached per document, since Qt asks for each of them
     * repeatedly while laying out and painting.
     */
    static void installResourceProvider( QTextDocument *document );

  private:
    //! URL scheme QgsProject references project attachments by.
    static constexpr QLatin1StringView sScheme { "attachment" };
    //! What any attachment reference starts with, in any spelling.
    static constexpr QLatin1StringView sIdentifierPrefix { "attachment:" };
    //! How much decoded image data one document's provider keeps, in bytes.
    static constexpr int sMaxCachedImageBytes = 64 * 1024 * 1024;

#ifndef SIP_RUN
    //! Whether replacements form their own undo step, or fold into the edit before them.
    enum class UndoStep
    {
      Separate,
      JoinPrevious,
    };

    //! One image to replace: the span it occupies, and the format to give it.
    struct ImageRewrite
    {
        int position = 0;
        int length = 0;
        QTextImageFormat format;
    };

    /**
     * Applies \a rewrites to \a document.
     *
     * Works back to front, so that replacing one image cannot shift the position
     * of the next, and one character at a time, because images that sit next to
     * each other with the same format arrive as a single fragment and replacing
     * that wholesale would merge them into one.
     */
    static void applyImageRewrites( QTextDocument *document, const QList<ImageRewrite> &rewrites, UndoStep undoStep );
#endif

    //! The one spelling QgsProject::resolveAttachmentIdentifier() accepts.
    static constexpr QLatin1StringView sCanonicalPrefix { "attachment:///" };

    /**
     * Decodes a data: image URL as QTextDocument writes one —
     * "data:image/<name>.<FORMAT>;base64,<payload>" — into \a image, reporting in
     * \a suffix the file suffix to store it under.
     */
    static bool decodeDataUrl( const QString &url, QImage &image, QString &suffix );

    KadasAttachmentUtils() = delete;
#ifdef SIP_RUN
    KadasAttachmentUtils();
#endif
};

#endif // KADASATTACHMENTUTILS_H
