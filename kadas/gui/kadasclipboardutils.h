/***************************************************************************
    kadasclipboardutils.h
    ---------------------
    copyright            : (C) 2026 by Denis Rouzaud
    email                : denis@opengis.ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef KADASCLIPBOARDUTILS_H
#define KADASCLIPBOARDUTILS_H

#include <QImage>

#define SIP_NO_FILE

/**
 * Utility to exchange a QImage with the system clipboard, using native macOS
 * NSPasteboard API on Apple platforms to avoid crashes in Apple's ImageIO
 * codecs (a libtiff symbol conflict with GDAL when encoding, and a PNG decoder
 * crash when the pasteboard transcodes screenshots when reading).
 */
class KadasClipboardUtils
{
  public:
    static void copyImageToClipboard( const QImage &image );

    /**
     * Returns the image currently held on the system clipboard, or a null
     * QImage if none is available. On macOS the raw bytes are read directly
     * from NSPasteboard and decoded with Qt, avoiding Apple's flavour
     * translation which crashes on some screenshots.
     */
    static QImage imageFromClipboard();
};

#endif // KADASCLIPBOARDUTILS_H
