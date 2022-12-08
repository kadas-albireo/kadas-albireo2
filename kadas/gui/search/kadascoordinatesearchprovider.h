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

#include <QRegExp>

#include <kadas/gui/kadassearchprovider.h>

class KADAS_GUI_EXPORT KadasCoordinateSearchProvider : public KadasSearchProvider
{
    Q_OBJECT
  public:
    KadasCoordinateSearchProvider( QgsMapCanvas *mapCanvas );
    void startSearch( const QString &searchtext, const SearchRegion &searchRegion ) override;

  private:
    QRegExp mPatLVDD;
    QRegExp mPatDM;
    QRegExp mPatDMS;
    QRegExp mPatUTM;
    QRegExp mPatUTM2;
    QRegExp mPatMGRS;

    static const QString sCategoryName;
    double parseNumber( const QString &string ) const;
};

#endif // KADASCOORDINATESEARCHPROVIDER_H
