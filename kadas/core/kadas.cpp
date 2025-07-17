/***************************************************************************
    kadas.cpp
    ---------
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

#include <gdal.h>
#include <cpl_string.h>
#include <openssl/bio.h>
#include <openssl/ssl.h>

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QNetworkProxyFactory>
#include <QSettings>
#include <QStandardPaths>

#include <qgis/qgsnetworkaccessmanager.h>
#include <qgis/qgsrasterlayer.h>
#include <qgis/qgssettings.h>

#include "kadas/core/kadas.h"
#include "kadas/core/kadas_config.h"

#ifdef Q_OS_WINDOWS
#include <windows.h>
#include <wincrypt.h>
#endif

const char *Kadas::KADAS_VERSION = _KADAS_VERSION_;
const int Kadas::KADAS_VERSION_INT = _KADAS_VERSION_INT_;
const char *Kadas::KADAS_RELEASE_NAME = _KADAS_NAME_;
const char *Kadas::KADAS_FULL_RELEASE_NAME = _KADAS_FULL_NAME_;
const char *Kadas::KADAS_DEV_VERSION = _KADAS_DEV_VERSION_;
const char *Kadas::KADAS_BUILD_DATE = __DATE__;

static QString resolveDataPath()
{
  QString dataPath = qgetenv( "KADAS_DATA_PATH" );
  if ( !dataPath.isEmpty() )
  {
    return dataPath;
  }
  QFile file( QDir( QApplication::applicationDirPath() ).absoluteFilePath( "kadassourcedir.txt" ) );
  if ( file.open( QIODevice::ReadOnly ) )
  {
    return QDir( file.readAll().trimmed() ).absoluteFilePath( "data" );
  }
  else
  {
    return QDir( QString( "%1/../share/kadas" ).arg( QApplication::applicationDirPath() ) ).absolutePath();
  }
}

static QString resolveResourcePath()
{
  QFile file( QDir( QApplication::applicationDirPath() ).absoluteFilePath( "kadassourcedir.txt" ) );
  if ( file.open( QIODevice::ReadOnly ) )
  {
    return QDir( file.readAll().trimmed() ).absoluteFilePath( "kadas/resources" );
  }
  else
  {
    return QDir( QString( "%1/../share/kadas/resources" ).arg( QApplication::applicationDirPath() ) ).absolutePath();
  }
}

static QByteArray X509_to_PEM( X509 *cert )
{
  BIO *bio = BIO_new( BIO_s_mem() );
  if ( NULL == bio )
  {
    return QByteArray();
  }

  if ( PEM_write_bio_X509( bio, cert ) != 1 )
  {
    BIO_free( bio );
    return QByteArray();
  }

  BUF_MEM *mem = NULL;
  BIO_get_mem_ptr( bio, &mem );
  if ( !mem || !mem->data || !mem->length )
  {
    BIO_free( bio );
    return QByteArray();
  }
  QByteArray pem( mem->data, mem->length );
  BIO_free( bio );
  return pem;
}


QString Kadas::configPath()
{
  QDir appDataDir = QDir( QStandardPaths::writableLocation( QStandardPaths::AppDataLocation ) );
  return appDataDir.absoluteFilePath( Kadas::KADAS_RELEASE_NAME );
}

QString Kadas::pkgDataPath()
{
  static QString dataPath = resolveDataPath();
  return dataPath;
}

QString Kadas::pkgResourcePath()
{
  static QString resourcePath = resolveResourcePath();
  return resourcePath;
}

QString Kadas::projectTemplatesPath()
{
  return QDir( pkgDataPath() ).absoluteFilePath( "project_templates" );
}

GDALDatasetH Kadas::gdalOpenForLayer( const QgsRasterLayer *layer, QString *errMsg )
{
  if ( !layer )
  {
    return nullptr;
  }

  QString providerType = layer->providerType();
  QString layerSource = layer->source();

  QgsDataSourceUri uri;
  uri.setEncodedUri( layerSource );
  if ( uri.hasParam( "url" ) )
  {
    // Set GDAL Proxy Env-Vars for URL
    QUrl url( uri.param( "url" ) );
    QSettings settings;
    QString gdalHttpProxy = settingsGdalProxyHttp->value();
    QString gdalProxyUserPwd = settingsGdalProxyUserPassword->value();
    QString gdalProxyAuth = settingsGdalProxyAuth->value();
    QString gdalHttpUserPwd = settingsGdalHttpUserPassword->value();
    QString gdalHttpAuth = settingsGdalHttpAuth->value();

    if ( !gdalHttpProxy.isEmpty() )
    {
      QgsDebugMsgLevel( QString( "GDAL_HTTP_PROXY: %1" ).arg( gdalHttpProxy ), 2 );
      qputenv( "GDAL_HTTP_PROXY", gdalHttpProxy.toLocal8Bit() );
      qputenv( "GDAL_HTTPS_PROXY", gdalHttpProxy.toLocal8Bit() );
    }
    else
    {
      QgsDebugMsgLevel( QString( "Unset GDAL_HTTP(S)_PROXY" ), 2 );
      qunsetenv( "GDAL_HTTP_PROXY" );
      qunsetenv( "GDAL_HTTPS_PROXY" );
    }

      if ( !gdalHttpProxy.isEmpty() )
      {
        qputenv( "GDAL_HTTP_PROXY", gdalHttpProxy.toLocal8Bit() );
      }
      else
      {
        qunsetenv( "GDAL_HTTP_PROXY" );
      }

      if ( !gdalProxyUserPwd.isEmpty() )
      {
        qputenv( "GDAL_HTTP_PROXYUSERPWD", gdalProxyUserPwd.toLocal8Bit() );
      }
      if ( !gdalProxyAuth.isEmpty() )
      {
        qputenv( "GDAL_PROXY_AUTH", gdalProxyAuth.toLocal8Bit() );
      }
    }

    if ( !gdalHttpUserPwd.isEmpty() )
    {
      qputenv( "GDAL_HTTP_USERPWD", gdalHttpUserPwd.toLocal8Bit() );
    }
    if ( !gdalHttpAuth.isEmpty() )
    {
      qputenv( "GDAL_HTTP_AUTH", gdalHttpAuth.toLocal8Bit() );
    }
  }

  if ( providerType == "gdal" )
  {
    return GDALOpen( layerSource.toUtf8().data(), GA_ReadOnly );
  }
  else if ( providerType == "wcs" )
  {
    QString wcsUrl;
    if ( !uri.hasParam( "url" ) )
    {
      return nullptr;
    }
    wcsUrl = uri.param( "url" );
    if ( !wcsUrl.endsWith( "?" ) && !wcsUrl.endsWith( "&" ) )
    {
      if ( wcsUrl.contains( "?" ) )
      {
        wcsUrl.append( "&" );
      }
      else
      {
        wcsUrl.append( "?" );
      }
    }
    QString gdalSource = QString( "WCS:%1" ).arg( wcsUrl );
    if ( uri.hasParam( "version" ) )
    {
      gdalSource.append( QString( "&version=%1" ).arg( uri.param( "version" ) ) );
    }
    if ( uri.hasParam( "identifier" ) )
    {
      if ( wcsUrl.contains( "MapServer", Qt::CaseInsensitive ) )
      {
        gdalSource.append( QString( "&coverage=Coverage%1" ).arg( uri.param( "identifier" ) ) );
      }
      else
      {
        gdalSource.append( QString( "&coverage=%1" ).arg( uri.param( "identifier" ) ) );
      }
    }
    if ( uri.hasParam( "crs" ) )
    {
      gdalSource.append( QString( "&crs=%1" ).arg( uri.param( "crs" ) ) );
    }
    QString cacheDir = QDir( QStandardPaths::writableLocation( QStandardPaths::CacheLocation ) ).absoluteFilePath( "wcs_cache" );
    char **papszOptions = nullptr;
    papszOptions = CSLSetNameValue( papszOptions, "CACHE", cacheDir.toLocal8Bit().data() );
    GDALDatasetH dataset = GDALOpenEx( gdalSource.toUtf8().data(), GA_ReadOnly, nullptr, papszOptions, nullptr );
    if ( !dataset && errMsg )
    {
      *errMsg = QApplication::tr( "Failed to open raster file: %1" ).arg( layerSource );
    }
    return dataset;
  }
  return nullptr;
}

void Kadas::importSslCertificates()
{
  // Look for certificates in <appDataDir>/certificates to add to the SSL socket CA certificate database
  QDir certDir( QDir( Kadas::pkgDataPath() ).absoluteFilePath( "certificates" ) );
  QgsDebugMsgLevel( QString( "Looking for certificates in %1" ).arg( certDir.absolutePath() ), 2 );
  for ( const QString &certFilename : certDir.entryList( QStringList() << "*.pem", QDir::Files ) )
  {
    QFile certFile( certDir.absoluteFilePath( certFilename ) );
    if ( certFile.open( QIODevice::ReadOnly ) )
    {
      QgsDebugMsgLevel( QString( "Reading certificate file %1" ).arg( certFile.fileName() ), 2 );
      QByteArray pem = certFile.readAll();
      QList<QSslCertificate> certs = QSslCertificate::fromData( pem, QSsl::Pem );
      QgsDebugMsgLevel( QString( "Adding %1 certificates" ).arg( certs.size() ), 2 );
#if QT_VERSION >= QT_VERSION_CHECK( 5, 15, 0 )
      QSslConfiguration::defaultConfiguration().addCaCertificates( certs );
#else
      for ( const QSslCertificate &cert : certs )
      {
        QSslSocket::addDefaultCaCertificate( cert );
      }
#endif
    }
  }

#ifdef Q_OS_WINDOWS
  // Taken from curl-7.77.0/lib/vtls/openssl.c
  HCERTSTORE hStore = CertOpenSystemStore( 0, TEXT( "ROOT" ) );

  if ( hStore )
  {
    PCCERT_CONTEXT pContext = NULL;
    /* The array of enhanced key usage OIDs will vary per certificate and is
       declared outside of the loop so that rather than malloc/free each
       iteration we can grow it with realloc, when necessary. */
    CERT_ENHKEY_USAGE *enhkey_usage = NULL;
    DWORD enhkey_usage_size = 0;

    /* This loop makes a best effort to import all valid certificates from
       the MS root store. If a certificate cannot be imported it is skipped. */
    for ( ;; )
    {
      X509 *x509;
      FILETIME now;
      BYTE key_usage[2];
      DWORD req_size;
      const unsigned char *encoded_cert;

      pContext = CertEnumCertificatesInStore( hStore, pContext );
      if ( !pContext )
        break;

      char cert_name[256];
      if ( !CertGetNameStringA( pContext, CERT_NAME_SIMPLE_DISPLAY_TYPE, 0, NULL, cert_name, sizeof( cert_name ) ) )
      {
        strcpy( cert_name, "Unknown" );
      }
      QgsDebugMsgLevel( QString( "Checking system certificate %1" ).arg( cert_name ), 2 );

      encoded_cert = ( const unsigned char * ) pContext->pbCertEncoded;
      if ( !encoded_cert )
        continue;

      GetSystemTimeAsFileTime( &now );
      if ( CompareFileTime( &pContext->pCertInfo->NotBefore, &now ) > 0 || CompareFileTime( &now, &pContext->pCertInfo->NotAfter ) > 0 )
        continue;

      /* If key usage exists check for signing attribute */
      if ( CertGetIntendedKeyUsage( pContext->dwCertEncodingType, pContext->pCertInfo, key_usage, sizeof( key_usage ) ) )
      {
        if ( !( key_usage[0] & CERT_KEY_CERT_SIGN_KEY_USAGE ) )
          continue;
      }
      else if ( GetLastError() )
        continue;

      /* If enhanced key usage exists check for server auth attribute.
       *
       * Note "In a Microsoft environment, a certificate might also have EKU
       * extended properties that specify valid uses for the certificate."
       * The call below checks both, and behavior varies depending on what is
       * found. For more details see CertGetEnhancedKeyUsage doc.
       */
      if ( CertGetEnhancedKeyUsage( pContext, 0, NULL, &req_size ) )
      {
        if ( req_size && req_size > enhkey_usage_size )
        {
          void *tmp = realloc( enhkey_usage, req_size );

          if ( !tmp )
          {
            // Out of memory allocating for OID list
            break;
          }

          enhkey_usage = ( CERT_ENHKEY_USAGE * ) tmp;
          enhkey_usage_size = req_size;
        }

        if ( CertGetEnhancedKeyUsage( pContext, 0, enhkey_usage, &req_size ) )
        {
          if ( !enhkey_usage->cUsageIdentifier )
          {
            /* "If GetLastError returns CRYPT_E_NOT_FOUND, the certificate is
               good for all uses. If it returns zero, the certificate has no
               valid uses." */
            if ( ( HRESULT ) GetLastError() != CRYPT_E_NOT_FOUND )
              continue;
          }
          else
          {
            DWORD i;
            bool found = false;

            for ( i = 0; i < enhkey_usage->cUsageIdentifier; ++i )
            {
              if ( !strcmp( "1.3.6.1.5.5.7.3.1" /* OID server auth */, enhkey_usage->rgpszUsageIdentifier[i] ) )
              {
                found = true;
                break;
              }
            }

            if ( !found )
              continue;
          }
        }
        else
          continue;
      }
      else
        continue;

      x509 = d2i_X509( NULL, &encoded_cert, pContext->cbCertEncoded );
      if ( !x509 )
        continue;

      QByteArray pem = X509_to_PEM( x509 );
      QList<QSslCertificate> certs = QSslCertificate::fromData( pem, QSsl::Pem );
      QgsDebugMsgLevel( QString( "Importing %1 certificates from system certificate %2" ).arg( certs.size() ).arg( cert_name ), 2 );
      QSslConfiguration::defaultConfiguration().addCaCertificates( certs );
      X509_free( x509 );
    }

    free( enhkey_usage );
    CertFreeCertificateContext( pContext );
    CertCloseStore( hStore, 0 );
  }
#endif // Q_OS_WINDOWS
}
