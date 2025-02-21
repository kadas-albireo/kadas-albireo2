/***************************************************************************
                        kadasalternategotofilter.h
                        ----------------------------
   begin                : February 2025
   copyright            : (C) 2025 by Denis Rouzaud
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


#ifndef KADASALTERNATEGOTOLOCATORFILTER_H
#define KADASALTERNATEGOTOLOCATORFILTER_H

#include <QRegularExpression>

#include "qgis/qgslocatorfilter.h"

class QgsMapCanvas;


class KadasAlternateGotoLocatorFilter : public QgsLocatorFilter
{
    Q_OBJECT

  public:
    KadasAlternateGotoLocatorFilter( QgsMapCanvas *mapCanvas, QObject *parent = nullptr );
    KadasAlternateGotoLocatorFilter *clone() const override;
    virtual QString name() const override { return QStringLiteral( "kadasgoto" ); }
    virtual QString displayName() const override { return tr( "Go to Coordinate" ); }
    virtual Priority priority() const override { return Medium; }
    QString prefix() const override { return QStringLiteral( "go" ); }
    QgsLocatorFilter::Flags flags() const override { return QgsLocatorFilter::FlagFast; }

    void fetchResults( const QString &string, const QgsLocatorContext &context, QgsFeedback *feedback ) override;
    void triggerResult( const QgsLocatorResult &result ) override;
    virtual void clearPreviousResults() override;

  private:
    double parseNumber( const QString &string ) const;
    bool matchOneOf( const QString &str, const QVector<QRegularExpression> &patterns, QRegularExpressionMatch &match ) const;

    QgsMapCanvas *mCanvas = nullptr;

    QRegularExpression mPatLVDD;
    QRegularExpression mPatLVDDalt;
    QRegularExpression mPatDM;
    QRegularExpression mPatDMalt;
    QRegularExpression mPatDMS;
    QRegularExpression mPatDMSalt;
    QRegularExpression mPatUTM;
    QRegularExpression mPatUTMalt;
    QRegularExpression mPatUTM2;
    QRegularExpression mPatMGRS;

    QString mPinItemId;
};

#endif // KADASALTERNATEGOTOLOCATORFILTER_H
