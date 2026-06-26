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

QImage KadasClipboardUtils::imageFromClipboard() {
  NSPasteboard *pasteboard = [NSPasteboard generalPasteboard];
  if (!pasteboard)
    return QImage();

  NSArray<NSPasteboardType> *available = [pasteboard types];
  if (available.count == 0)
    return QImage();

  // Read the raw encoded bytes of a flavour that is actually present and decode
  // them with Qt's own image readers. This avoids QMimeData::imageData() /
  // QClipboard::image(), which make the pasteboard transcode the image via
  // Apple's ImageIO (PasteboardCopyItemFlavorData -> CreateImageFromImage).
  // That translation crashes (SIGBUS) on some screenshots.
  NSMutableArray<NSPasteboardType> *candidates = [NSMutableArray array];
  for (NSPasteboardType preferred in @[
         NSPasteboardTypePNG, NSPasteboardTypeTIFF, @"public.jpeg",
         @"com.compuserve.gif"
       ]) {
    if ([available containsObject:preferred])
      [candidates addObject:preferred];
  }
  for (NSPasteboardType type in available) {
    if (![candidates containsObject:type])
      [candidates addObject:type];
  }

  for (NSPasteboardType type in candidates) {
    NSData *data = [pasteboard dataForType:type];
    if (!data || data.length == 0)
      continue;
    const QByteArray bytes(static_cast<const char *>(data.bytes),
                           static_cast<int>(data.length));
    const QImage image = QImage::fromData(bytes);
    if (!image.isNull())
      return image;
  }

  return QImage();
}
