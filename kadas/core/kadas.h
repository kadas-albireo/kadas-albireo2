/***************************************************************************
    kadas.h
    -------
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

#ifndef KADAS_H
#define KADAS_H

#include <QString>

#include <kadas/core/kadas_core.h>

class KADAS_CORE_EXPORT Kadas
{
  public:
    // Version string
    static const char *KADAS_VERSION;
    // Version number used for comparing versions using the "Check QGIS Version" function
    static const int KADAS_VERSION_INT;
    // Release name
    static const char *KADAS_RELEASE_NAME;
    // Full release name
    static const char *KADAS_FULL_RELEASE_NAME;
    // The development version
    static const char *KADAS_DEV_VERSION;
    // The build date
    static const char *KADAS_BUILD_DATE;

    // Path where user-configuration is stored
    static QString configPath();

    // Path where application data is stored
    static QString pkgDataPath();

    // Path where project templates are stored
    static QString projectTemplatesPath();
};

#endif // KADAS_H
