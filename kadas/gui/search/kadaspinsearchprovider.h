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

#include <kadas/gui/kadassearchprovider.h>

class KADAS_GUI_EXPORT KadasPinSearchProvider : public KadasSearchProvider
{
    Q_OBJECT
  public:
    KadasPinSearchProvider( QgsMapCanvas *mapCanvas ) : KadasSearchProvider( mapCanvas ) {}
    void startSearch( const QString &searchtext, const SearchRegion &searchRegion ) override;

  private:
    static const QString sCategoryName;
};

#endif // KADASPINSEARCHPROVIDER_H
