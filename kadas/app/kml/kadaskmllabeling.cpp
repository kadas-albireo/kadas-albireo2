/***************************************************************************
    kadaskmllabeling.cpp
    --------------------
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

#include <QTextStream>

#include <qgis/qgsfeature.h>
#include <qgis/qgslabelposition.h>
#include <qgis/labelposition.h>
#include <qgis/feature.h>

#include <kml/kadaskmlexport.h>
#include <kml/kadaskmllabeling.h>


KadasKMLLabelProvider::KadasKMLLabelProvider( QTextStream *outStream, QgsVectorLayer *layer, const QgsPalLayerSettings *settings )
  : QgsVectorLayerLabelProvider( layer, QString(), false, settings ), mOutStream( outStream )
{
}

void KadasKMLLabelProvider::drawLabel( QgsRenderContext &context, pal::LabelPosition *label ) const
{
  if ( !mOutStream )
  {
    return;
  }

  QgsPalLayerSettings tmpLyr( mSettings );

  QString text = label->getFeaturePart()->feature()->labelText();

  QColor fontColor = tmpLyr.format().color();

  double labelMidPointX = label->getX() + label->getWidth() / 2.0;
  double labelMidPointY = label->getY() + label->getHeight() / 2.0;

  ( *mOutStream ) << QString( "<Placemark><name>%1</name><Style><IconStyle><scale>0</scale></IconStyle><LabelStyle><color>%2</color></LabelStyle></Style><Point><coordinates>%3,%4</coordinates></Point></Placemark>" )
                       .arg( text )
                       .arg( KadasKMLExport::convertColor( fontColor ) )
                       .arg( QString::number( labelMidPointX ) )
                       .arg( QString::number( labelMidPointY ) );
  ( *mOutStream ) << "\n";
}
