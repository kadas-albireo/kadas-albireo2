/***************************************************************************
    kadascoordinatesearchprovider.h
    -------------------------------
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

#ifndef KADASCOORDINATESEARCHPROVIDER_H
#define KADASCOORDINATESEARCHPROVIDER_H

#include <QRegularExpression>

#include <kadas/gui/kadassearchprovider.h>

class KADAS_GUI_EXPORT KadasCoordinateSearchProvider : public KadasSearchProvider
{
    Q_OBJECT
  public:
    KadasCoordinateSearchProvider( QgsMapCanvas *mapCanvas );
    void startSearch( const QString &searchtext, const SearchRegion &searchRegion ) override;

  private:
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

    static const QString sCategoryName;
    double parseNumber( const QString &string ) const;
    bool matchOneOf( const QString &str, const QVector<QRegularExpression> &patterns, QRegularExpressionMatch &match ) const;
};

#endif // KADASCOORDINATESEARCHPROVIDER_H
