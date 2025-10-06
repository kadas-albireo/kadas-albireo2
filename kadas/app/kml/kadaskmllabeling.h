/***************************************************************************
    kadaskmllabeling.h
    ------------------
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

#ifndef KADASKMLLABELING_H
#define KADASKMLLABELING_H

#include <qgis/qgsrendercontext.h>
#include <qgis/qgsvectorlayerlabelprovider.h>

class QTextStream;

class KadasKMLLabelProvider : public QgsVectorLayerLabelProvider {
public:
  KadasKMLLabelProvider(QTextStream *outStream, QgsVectorLayer *layer,
                        const QgsPalLayerSettings *settings);

  void drawLabel(QgsRenderContext &context,
                 pal::LabelPosition *label) const override;

private:
  QTextStream *mOutStream;

  KadasKMLLabelProvider(); // private
};

#endif // KADASKMLLABELING_H
