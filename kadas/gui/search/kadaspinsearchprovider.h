/***************************************************************************
    kadaspinsearchprovider.h
    ------------------------
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

#ifndef KADASPINSEARCHPROVIDER_H
#define KADASPINSEARCHPROVIDER_H

#include <qgis/qgslocatorfilter.h>

#include <kadas/gui/kadas_gui.h>

class QgsMapCanvas;


class KADAS_GUI_EXPORT KadasPinSearchProvider : public QgsLocatorFilter
{
    Q_OBJECT
  public:
    KadasPinSearchProvider( QgsMapCanvas *mapCanvas );

    virtual QgsLocatorFilter *clone() const override;
    QString name() const override { return QStringLiteral( "pins" ); }
    QString displayName() const override { return tr( "Pins" ); }
    virtual Priority priority() const override { return Priority::High; }
    virtual void fetchResults( const QString &string, const QgsLocatorContext &context, QgsFeedback *feedback ) override;
    virtual void triggerResult( const QgsLocatorResult &result ) override;

  private:
    QgsMapCanvas *mMapCanvas = nullptr;
};

#endif // KADASPINSEARCHPROVIDER_H
