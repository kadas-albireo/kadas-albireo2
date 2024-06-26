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

#include <qgis/qgssettingsentryimpl.h>

#include <kadas/core/kadas_core.h>
#include <kadas/core/kadassettingstree.h>


class QUrl;
class QgsMapLayer;
class QgsRasterLayer;

typedef void *GDALDatasetH;

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


    static inline QgsSettingsTreeNode *sTreeGdalProxy = KadasSettingsTree::treeRoot()->createChildNode( QStringLiteral( "gdal-proxy" ) );
    static const inline QgsSettingsEntryString *settingsGdalProxyHttp = new QgsSettingsEntryString(QStringLiteral("http-proxy"), sTreeGdalProxy, QString(), QStringLiteral("this will be used to set GDAL_HTTP_PROXY env variable.") ) SIP_SKIP;
    static const inline QgsSettingsEntryString *settingsGdalProxyUserPassword = new QgsSettingsEntryString(QStringLiteral("user-password"), sTreeGdalProxy, QString(), QStringLiteral("this will be used to set GDAL_HTTP_PROXYUSERPWD env variable.") ) SIP_SKIP;
    static const inline QgsSettingsEntryString *settingsGdalProxyAuth = new QgsSettingsEntryString(QStringLiteral("auth"), sTreeGdalProxy, QString(), QStringLiteral("this will be used to set GDAL_HTTP_PROXY_AUTH env variable.") ) SIP_SKIP;

    // Path where user-configuration is stored
    static QString configPath();

    // Path where application data is stored
    static QString pkgDataPath();

    // Path where application resources are stored
    static QString pkgResourcePath();

    // Path where project templates are stored
    static QString projectTemplatesPath();

    // Returns gdal source string for raster layer or null string in case of error
    static GDALDatasetH gdalOpenForLayer( const QgsRasterLayer *layer, QString *errMsg = nullptr );

    // Import SSL certificates from the certificate directory and from the system store (on Windows)
    static void importSslCertificates();
};

#endif // KADAS_H
