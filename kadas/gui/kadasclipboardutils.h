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
 * Utility to copy a QImage to the system clipboard, using native macOS
 * NSPasteboard API on Apple platforms to avoid a crash in ImageIO's TIFF
 * encoder caused by a libtiff symbol conflict with GDAL.
 */
namespace KadasClipboardUtils
{
  void copyImageToClipboard( const QImage &image );
}

#endif // KADASCLIPBOARDUTILS_H
