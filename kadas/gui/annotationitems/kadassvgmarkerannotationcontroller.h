/***************************************************************************
    kadassvgmarkerannotationcontroller.h
    ------------------------------------
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

#ifndef KADASSVGMARKERANNOTATIONCONTROLLER_H
#define KADASSVGMARKERANNOTATIONCONTROLLER_H

#include "kadas/gui/annotationitems/kadasmarkerannotationcontroller.h"

//! Controller for KadasSvgMarkerAnnotationItem (type id "kadas:svgmarker").
class KADAS_GUI_EXPORT KadasSvgMarkerAnnotationController : public KadasMarkerAnnotationController
{
  public:
    KadasSvgMarkerAnnotationController() = default;

    QString itemType() const override;
    QString itemName() const override;
    QgsAnnotationItem *createItem() const override;

    KadasAnnotationStyleEditor *createStyleEditor( QWidget *parent = nullptr ) const override;

    void applyPersistedStyle( QgsAnnotationItem *item ) const override;
    void persistStyle( const QgsAnnotationItem *item ) const override;

#ifndef SIP_RUN
    static const QgsSettingsEntryString *settingsSvgPath;
    static const QgsSettingsEntryInteger *settingsSvgSize;
    static const QgsSettingsEntryColor *settingsSvgFillColor;
#endif
};

#endif // KADASSVGMARKERANNOTATIONCONTROLLER_H
