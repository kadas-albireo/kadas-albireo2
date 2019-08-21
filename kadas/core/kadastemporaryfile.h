/***************************************************************************
    kadastemporaryfile.h
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

#ifndef KADASTEMPORARYFILE_H
#define KADASTEMPORARYFILE_H

#include <QString>

#include <kadas/core/kadas_core.h>

class KADAS_CORE_EXPORT KadasTemporaryFile
{
  public:
    static QString createNewFile( const QString &templateName );
    static void clear();
};

#endif // KADASTEMPORARYFILE_H
