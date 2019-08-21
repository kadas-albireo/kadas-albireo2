/***************************************************************************
    kadasclipboard.cpp
    ------------------
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

#include <QApplication>
#include <QClipboard>
#include <QDataStream>
#include <QMimeData>
#include <QSettings>

#include <qgis/qgsfeaturestore.h>
#include <qgis/qgsgeometry.h>

#include <kadas/gui/kadasclipboard.h>

KadasClipboard::KadasClipboard( QObject *parent )
  : QObject( parent )
{
  connect( QApplication::clipboard(), &QClipboard::dataChanged, this, &KadasClipboard::onDataChanged );
}

void KadasClipboard::setMimeData( QMimeData *mimeData )
{
  QApplication::clipboard()->setMimeData( mimeData );
  mFeatureStore = QgsFeatureStore();
}

const QMimeData *KadasClipboard::mimeData()
{
  return QApplication::clipboard()->mimeData();
}

bool KadasClipboard::isEmpty() const
{
  const QMimeData *mimeData = QApplication::clipboard()->mimeData();
  return !mimeData || mimeData->formats().isEmpty();
}

bool KadasClipboard::hasFormat( const QString &format ) const
{
  if ( format == KADASCLIPBOARD_FEATURESTORE_MIME )
  {
    return !mFeatureStore.features().isEmpty();
  }
  const QMimeData *mimeData = QApplication::clipboard()->mimeData();
  return mimeData && mimeData->hasFormat( format );
}

void KadasClipboard::setStoredFeatures( const QgsFeatureStore &featureStore )
{
  // Also store plaintext version
  QSettings settings;
  bool copyWKT = settings.value( "Qgis/copyGeometryAsWKT", true ).toBool();

  QStringList textLines;
  QStringList textFields;

  // first column names
  if ( copyWKT )
  {
    textFields += "wkt_geom";
  }
  for ( int i = 0, n = featureStore.fields().count(); i < n; ++i )
  {
    textFields.append( featureStore.fields().at( i ).name() );
  }
  textLines.append( textFields.join( "\t" ) );
  textFields.clear();

  // then the field contents
  for ( const QgsFeature &feature : featureStore.features() )
  {
    if ( copyWKT )
    {
      if ( feature.hasGeometry() )
      {
        textFields.append( feature.geometry().asWkt() );
      }
      else
      {
        textFields.append( settings.value( "Qgis/nullValue", "NULL" ).toString() );
      }
    }
    for ( const QVariant &attr : feature.attributes() )
    {
      textFields.append( attr.toString() );
    }
    textLines.append( textFields.join( "\t" ) );
    textFields.clear();
  }
  QMimeData *mimeData = new QMimeData();
  mimeData->setData( "text/plain", textLines.join( "\n" ).toLocal8Bit() );
  QApplication::clipboard()->setMimeData( mimeData );

  // Make QApplication::clipboard() emit the dataChanged signal
  QApplication::processEvents( QEventLoop::ExcludeUserInputEvents );

  // After plaintext version, because dataChanged clears the internal feature store
  mFeatureStore = featureStore;
}

const QgsFeatureStore &KadasClipboard::getStoredFeatures() const
{
  return mFeatureStore;
}

void KadasClipboard::onDataChanged()
{
  mFeatureStore = QgsFeatureStore();
  emit dataChanged();
}
