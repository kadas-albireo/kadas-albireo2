/***************************************************************************
    kadassvgmarkerannotationitem.h
    ------------------------------
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

#ifndef KADASSVGMARKERANNOTATIONITEM_H
#define KADASSVGMARKERANNOTATIONITEM_H

#include <QString>

#include <qgis/qgsannotationmarkeritem.h>

#include "kadas/gui/kadas_gui.h"

//! "Custom SVG marker" annotation item (type id "kadas:svgmarker").
class KADAS_GUI_EXPORT KadasSvgMarkerAnnotationItem : public QgsAnnotationMarkerItem
{
  public:
    KadasSvgMarkerAnnotationItem( const QgsPoint &point = QgsPoint() );

    static QString itemTypeId() { return QStringLiteral( "kadas:svgmarker" ); }

    //! qrc path of the question-mark placeholder used until the user picks an SVG.
    static QString placeholderIconPath();

    QString type() const override;
    KadasSvgMarkerAnnotationItem *clone() const override;

    static KadasSvgMarkerAnnotationItem *create();

  private:
    void installDefaultSymbol();
};

#endif // KADASSVGMARKERANNOTATIONITEM_H
