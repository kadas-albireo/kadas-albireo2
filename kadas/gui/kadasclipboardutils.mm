/***************************************************************************
    kadasclipboardutils.mm
    ----------------------
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

#include "kadas/gui/kadasclipboardutils.h"

#include <QBuffer>

#import <AppKit/AppKit.h>

void KadasClipboardUtils::copyImageToClipboard(const QImage &image) {
  // Encode image as PNG in memory
  QByteArray pngData;
  QBuffer buffer(&pngData);
  buffer.open(QIODevice::WriteOnly);
  image.save(&buffer, "PNG");
  buffer.close();

  // Use NSPasteboard directly to avoid Qt's QMacMimeTiff converter, which
  // triggers Apple ImageIO's TIFF encoder. That encoder crashes due to a
  // libtiff symbol conflict with GDAL's libtiff in the same process.
  NSData *data = [NSData dataWithBytes:pngData.constData()
                                length:pngData.size()];
  NSPasteboard *pasteboard = [NSPasteboard generalPasteboard];
  [pasteboard clearContents];
  [pasteboard setData:data forType:NSPasteboardTypePNG];
}
