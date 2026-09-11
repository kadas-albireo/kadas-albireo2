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

#include <QHash>

#include <qgis/qgslocatorfilter.h>

#include "kadas/gui/kadas_gui.h"

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
    //! remarks as plain text, reusing the last conversion of that same markup.
    QString plainRemarks( const QString &remarks );

    QgsMapCanvas *mMapCanvas = nullptr;
    /**
     * Resolving markup to text costs far more than the match it feeds, and the
     * locator re-runs this filter from scratch on every keystroke over
     * descriptions that have not changed. Keyed by the markup itself, so an
     * edited description is simply a new entry rather than a stale one.
     */
    QHash<QString, QString> mPlainRemarks;
    //! How many conversions to keep before starting over; a project's pins come and go.
    static constexpr int sMaxCachedRemarks = 512;
};

#endif // KADASPINSEARCHPROVIDER_H
