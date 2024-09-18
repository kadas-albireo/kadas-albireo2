/***************************************************************************
    kadasglobedialog.cpp
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

#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QMenu>
#include <QTimer>

#include <qgis/qgsnetworkaccessmanager.h>
#include <qgis/qgslogger.h>
#include <qgis/qgsproject.h>
#include <qgis/qgssettings.h>

#include <osg/DisplaySettings>
#include <osgViewer/Viewer>
#include <osgEarth/Version>
#include <osgEarthUtil/EarthManipulator>

#include "kadas/core/kadas.h"
#include "kadasapplication.h"
#include <globe/kadasglobedialog.h>
#include <globe/kadasglobeintegration.h>

KadasGlobeDialog::KadasGlobeDialog( QWidget *parent, Qt::WindowFlags fl )
  : QDialog( parent, fl )
{
  setupUi( this );

  QMenu *addImageryMenu = new QMenu( this );

  QMenu *tmsImageryMenu = new QMenu( this );
  tmsImageryMenu->addAction( "Readymap: NASA BlueMarble Imagery", this, &KadasGlobeDialog::addTMSImagery )->setData( "http://readymap.org/readymap/tiles/1.0.0/1/" );
  tmsImageryMenu->addAction( "Readymap: NASA BlueMarble, ocean only", this, &KadasGlobeDialog::addTMSImagery )->setData( "http://readymap.org/readymap/tiles/1.0.0/2/" );
  tmsImageryMenu->addAction( "Readymap: High resolution insets from various world locations", this, &KadasGlobeDialog::addTMSImagery )->setData( "http://readymap.org/readymap/tiles/1.0.0/3/" );
  tmsImageryMenu->addAction( "Readymap: Global Land Cover Facility 15m Landsat", this, &KadasGlobeDialog::addTMSImagery )->setData( "http://readymap.org/readymap/tiles/1.0.0/6/" );
  tmsImageryMenu->addAction( "Readymap: NASA BlueMarble + Landsat + Ocean Masking Layer", this, &KadasGlobeDialog::addTMSImagery )->setData( "http://readymap.org/readymap/tiles/1.0.0/7/" );
  tmsImageryMenu->addAction( tr( "Custom..." ), this, &KadasGlobeDialog::addCustomTMSImagery );
  addImageryMenu->addAction( tr( "TMS" ) )->setMenu( tmsImageryMenu );

  QMenu *wmsImageryMenu = new QMenu( this );
  wmsImageryMenu->addAction( tr( "Custom..." ), this, &KadasGlobeDialog::addCustomWMSImagery );
  addImageryMenu->addAction( tr( "WMS" ) )->setMenu( wmsImageryMenu );

  QMenu *fileImageryMenu = new QMenu( this );
  QString worldtif = QDir::cleanPath( Kadas::pkgResourcePath() + "/globe/world.tif" );
  fileImageryMenu->addAction( tr( "world.tif" ), this, &KadasGlobeDialog::addRasterImagery )->setData( worldtif );
  fileImageryMenu->addAction( tr( "Custom..." ), this, &KadasGlobeDialog::addCustomRasterImagery );
  addImageryMenu->addAction( tr( "Raster" ) )->setMenu( fileImageryMenu );

  mAddImageryButton->setMenu( addImageryMenu );


  QMenu *addElevationMenu = new QMenu( this );

  QMenu *tmsElevationMenu = new QMenu( this );
  tmsElevationMenu->addAction( "Readymap: SRTM 90m Elevation Data", this, &KadasGlobeDialog::addTMSElevation )->setData( "http://readymap.org/readymap/tiles/1.0.0/9/" );
  tmsElevationMenu->addAction( tr( "Custom..." ), this, &KadasGlobeDialog::addCustomTMSElevation );
  addElevationMenu->addAction( tr( "TMS" ) )->setMenu( tmsElevationMenu );

  QMenu *fileElevationMenu = new QMenu( this );
  fileElevationMenu->addAction( tr( "Custom..." ), this, &KadasGlobeDialog::addCustomRasterElevation );
  addElevationMenu->addAction( tr( "Raster" ) )->setMenu( fileElevationMenu );

  mAddElevationButton->setMenu( addElevationMenu );


  comboBoxStereoMode->addItem( "OFF", -1 );
  comboBoxStereoMode->addItem( "ANAGLYPHIC", osg::DisplaySettings::ANAGLYPHIC );
  comboBoxStereoMode->addItem( "QUAD_BUFFER", osg::DisplaySettings::ANAGLYPHIC );
  comboBoxStereoMode->addItem( "HORIZONTAL_SPLIT", osg::DisplaySettings::HORIZONTAL_SPLIT );
  comboBoxStereoMode->addItem( "VERTICAL_SPLIT", osg::DisplaySettings::VERTICAL_SPLIT );

  lineEditAASamples->setValidator( new QIntValidator( lineEditAASamples ) );

#if OSGEARTH_VERSION_LESS_THAN( 2, 5, 0 )
  mSpinBoxVerticalScale->setVisible( false );
#endif

  connect( checkBoxSkyAutoAmbient, &QCheckBox::toggled, horizontalSliderMinAmbient, &QSlider::setEnabled );
  connect( checkBoxDateTime, &QCheckBox::toggled, dateTimeEditSky, &QSlider::setEnabled );
  connect( buttonBox->button( QDialogButtonBox::Apply ), &QPushButton::clicked, this, &KadasGlobeDialog::apply );

  restoreSavedSettings();
  readProjectSettings();
}

void KadasGlobeDialog::restoreSavedSettings()
{
  QgsSettings settings;

  // Video settings
  comboBoxStereoMode->setCurrentIndex( comboBoxStereoMode->findText( settings.value( "/Globe/stereoMode", "OFF" ).toString() ) );
  spinBoxStereoScreenDistance->setValue( settings.value( "/Globe/spinBoxStereoScreenDistance",
                                         osg::DisplaySettings::instance()->getScreenDistance() ).toDouble() );
  spinBoxStereoScreenWidth->setValue( settings.value( "/Globe/spinBoxStereoScreenWidth",
                                      osg::DisplaySettings::instance()->getScreenWidth() ).toDouble() );
  spinBoxStereoScreenHeight->setValue( settings.value( "/Globe/spinBoxStereoScreenHeight",
                                       osg::DisplaySettings::instance()->getScreenHeight() ).toDouble() );
  spinBoxStereoEyeSeparation->setValue( settings.value( "/Globe/spinBoxStereoEyeSeparation",
                                        osg::DisplaySettings::instance()->getEyeSeparation() ).toDouble() );
  spinBoxSplitStereoHorizontalSeparation->setValue( settings.value( "/Globe/spinBoxSplitStereoHorizontalSeparation",
      osg::DisplaySettings::instance()->getSplitStereoHorizontalSeparation() ).toInt() );
  spinBoxSplitStereoVerticalSeparation->setValue( settings.value( "/Globe/spinBoxSplitStereoVerticalSeparation",
      osg::DisplaySettings::instance()->getSplitStereoVerticalSeparation() ).toInt() );
  comboBoxSplitStereoHorizontalEyeMapping->setCurrentIndex( settings.value( "/Globe/comboBoxSplitStereoHorizontalEyeMapping",
      osg::DisplaySettings::instance()->getSplitStereoHorizontalEyeMapping() ).toInt() );
  comboBoxSplitStereoVerticalEyeMapping->setCurrentIndex( settings.value( "/Globe/comboBoxSplitStereoVerticalEyeMapping",
      osg::DisplaySettings::instance()->getSplitStereoVerticalEyeMapping() ).toInt() );
  groupBoxAntiAliasing->setChecked( settings.value( "/Globe/anti-aliasing", false ).toBool() );
  lineEditAASamples->setText( settings.value( "/Globe/anti-aliasing-level", "" ).toString() );

  horizontalSliderMinAmbient->setEnabled( checkBoxSkyAutoAmbient->isChecked() );
  dateTimeEditSky->setEnabled( checkBoxDateTime->isChecked() );

  // Advanced
  sliderScrollSensitivity->setValue( settings.value( "/Globe/scrollSensitivity", 20 ).toInt() );
  checkBoxInvertScroll->setChecked( settings.value( "/Globe/invertScrollWheel", 0 ).toInt() );
}

void KadasGlobeDialog::on_buttonBox_accepted()
{
  apply();
  accept();
}

void KadasGlobeDialog::on_buttonBox_rejected()
{
  restoreSavedSettings();
  readProjectSettings();
  reject();
}

void KadasGlobeDialog::apply()
{
  QgsSettings settings;

  // Video settings
  settings.setValue( "/Globe/stereoMode", comboBoxStereoMode->currentText() );
  settings.setValue( "/Globe/stereoScreenDistance", spinBoxStereoScreenDistance->value() );
  settings.setValue( "/Globe/stereoScreenWidth", spinBoxStereoScreenWidth->value() );
  settings.setValue( "/Globe/stereoScreenHeight", spinBoxStereoScreenHeight->value() );
  settings.setValue( "/Globe/stereoEyeSeparation", spinBoxStereoEyeSeparation->value() );
  settings.setValue( "/Globe/SplitStereoHorizontalSeparation", spinBoxSplitStereoHorizontalSeparation->value() );
  settings.setValue( "/Globe/SplitStereoVerticalSeparation", spinBoxSplitStereoVerticalSeparation->value() );
  settings.setValue( "/Globe/SplitStereoHorizontalEyeMapping", comboBoxSplitStereoHorizontalEyeMapping->currentIndex() );
  settings.setValue( "/Globe/SplitStereoVerticalEyeMapping", comboBoxSplitStereoVerticalEyeMapping->currentIndex() );
  settings.setValue( "/Globe/anti-aliasing", groupBoxAntiAliasing->isChecked() );
  settings.setValue( "/Globe/anti-aliasing-level", lineEditAASamples->text() );

  // Advanced settings
  settings.setValue( "/Globe/scrollSensitivity", sliderScrollSensitivity->value() );
  settings.setValue( "/Globe/invertScrollWheel", checkBoxInvertScroll->checkState() );

  writeProjectSettings();

  // Apply stereo settings
  int stereoMode = comboBoxStereoMode->currentData().toInt();
  if ( stereoMode == -1 )
  {
    osg::DisplaySettings::instance()->setStereo( false );
  }
  else
  {
    osg::DisplaySettings::instance()->setStereo( true );
    osg::DisplaySettings::instance()->setStereoMode(
      static_cast<osg::DisplaySettings::StereoMode>( stereoMode ) );
    osg::DisplaySettings::instance()->setEyeSeparation( spinBoxStereoEyeSeparation->value() );
    osg::DisplaySettings::instance()->setScreenDistance( spinBoxStereoScreenDistance->value() );
    osg::DisplaySettings::instance()->setScreenWidth( spinBoxStereoScreenWidth->value() );
    osg::DisplaySettings::instance()->setScreenHeight( spinBoxStereoScreenHeight->value() );
    osg::DisplaySettings::instance()->setSplitStereoVerticalSeparation(
      spinBoxSplitStereoVerticalSeparation->value() );
    osg::DisplaySettings::instance()->setSplitStereoVerticalEyeMapping(
      static_cast<osg::DisplaySettings::SplitStereoVerticalEyeMapping>(
        comboBoxSplitStereoVerticalEyeMapping->currentIndex() ) );
    osg::DisplaySettings::instance()->setSplitStereoHorizontalSeparation(
      spinBoxSplitStereoHorizontalSeparation->value() );
    osg::DisplaySettings::instance()->setSplitStereoHorizontalEyeMapping(
      static_cast<osg::DisplaySettings::SplitStereoHorizontalEyeMapping>(
        comboBoxSplitStereoHorizontalEyeMapping->currentIndex() ) );
  }

  emit settingsApplied();
}

void KadasGlobeDialog::readProjectSettings()
{
  // Imagery settings
  mImageryTreeView->clear();
  for ( const LayerDataSource &ds : getImageryDataSources() )
  {
    QTreeWidgetItem *item = new QTreeWidgetItem( QStringList() << ds.type << ds.uri );
    item->setFlags( item->flags() & ~Qt::ItemIsDropEnabled );
    mImageryTreeView->addTopLevelItem( item );
  }
  mImageryTreeView->resizeColumnToContents( 0 );

  // Elevation settings
  mElevationTreeView->clear();
  for ( const LayerDataSource &ds : getElevationDataSources() )
  {
    QTreeWidgetItem *item = new QTreeWidgetItem( QStringList() << ds.type << ds.uri );
    item->setFlags( item->flags() & ~Qt::ItemIsDropEnabled );
    mElevationTreeView->addTopLevelItem( item );
  }
  mElevationTreeView->resizeColumnToContents( 0 );

#if OSGEARTH_VERSION_GREATER_OR_EQUAL( 2, 5, 0 )
  mSpinBoxVerticalScale->setValue( QgsProject::instance()->readDoubleEntry( "Globe-Plugin", "/verticalScale", 1 ) );
#endif

  // Map settings
  groupBoxSky->setChecked( QgsProject::instance()->readBoolEntry( "Globe-Plugin", "/skyEnabled", true ) );
  checkBoxDateTime->setChecked( QgsProject::instance()->readBoolEntry( "Globe-Plugin", "/overrideDateTime", false ) );
  dateTimeEditSky->setDateTime( QDateTime::fromString( QgsProject::instance()->readEntry( "Globe-Plugin", "/skyDateTime", QDateTime::currentDateTime().toString() ) ) );
  checkBoxSkyAutoAmbient->setChecked( QgsProject::instance()->readBoolEntry( "Globe-Plugin", "/skyAutoAmbient", false ) );
  horizontalSliderMinAmbient->setValue( QgsProject::instance()->readDoubleEntry( "Globe-Plugin", "/skyMinAmbient", 30. ) );
}

void KadasGlobeDialog::writeProjectSettings()
{
  // Imagery settings
  QgsProject::instance()->removeEntry( "Globe-Plugin", "/imageryDatasources/" );
  for ( int row = 0, nRows = mImageryTreeView->topLevelItemCount(); row < nRows; ++row )
  {
    QTreeWidgetItem *item = mImageryTreeView->topLevelItem( row );
    QString key = QString( "/imageryDatasources/L%1" ).arg( row );
    QgsProject::instance()->writeEntry( "Globe-Plugin", key + "/type", item->text( 0 ) );
    QgsProject::instance()->writeEntry( "Globe-Plugin", key + "/uri", QgsProject::instance()->writePath( item->text( 1 ) ) );
  }

  // Elevation settings
  QgsProject::instance()->removeEntry( "Globe-Plugin", "/elevationDatasources/" );
  for ( int row = 0, nRows = mElevationTreeView->topLevelItemCount(); row < nRows; ++row )
  {
    QTreeWidgetItem *item = mElevationTreeView->topLevelItem( row );
    QString key = QString( "/elevationDatasources/L%1" ).arg( row );
    QgsProject::instance()->writeEntry( "Globe-Plugin", key + "/type", item->text( 0 ) );
    QgsProject::instance()->writeEntry( "Globe-Plugin", key + "/uri", QgsProject::instance()->writePath( item->text( 1 ) ) );
  }

#if OSGEARTH_VERSION_GREATER_OR_EQUAL( 2, 5, 0 )
  QgsProject::instance()->writeEntry( "Globe-Plugin", "/verticalScale", mSpinBoxVerticalScale->value() );
#endif

  // Map settings
  QgsProject::instance()->writeEntry( "Globe-Plugin", "/skyEnabled/", groupBoxSky->isChecked() );
  QgsProject::instance()->writeEntry( "Globe-Plugin", "/overrideDateTime/", checkBoxDateTime->isChecked() );
  QgsProject::instance()->writeEntry( "Globe-Plugin", "/skyDateTime/", dateTimeEditSky->dateTime().toString() );
  QgsProject::instance()->writeEntry( "Globe-Plugin", "/skyAutoAmbient/", checkBoxSkyAutoAmbient->isChecked() );
  QgsProject::instance()->writeEntry( "Globe-Plugin", "/skyMinAmbient/", horizontalSliderMinAmbient->value() );
}

bool KadasGlobeDialog::validateRemoteUri( const QString &uri, QString &errMsg ) const
{
  QUrl url( uri );
  QgsNetworkAccessManager *nam = QgsNetworkAccessManager::instance();
  QNetworkReply *reply = nullptr;

  while ( true )
  {
    QNetworkRequest req( url );
    req.setRawHeader( "User-Agent", "Wget/1.13.4" );
    reply = nam->get( req );
    QTimer timer;
    QEventLoop loop;
    QObject::connect( &timer, &QTimer::timeout, &loop, &QEventLoop::quit );
    QObject::connect( reply, &QNetworkReply::finished, &loop, &QEventLoop::quit );
    timer.setSingleShot( true );
    timer.start( 500 );
    loop.exec();
    if ( reply->isRunning() )
    {
      // Timeout
      reply->close();
      delete reply;
      errMsg = tr( "Timeout" );
      return false;
    }

    QUrl redirectUrl = reply->attribute( QNetworkRequest::RedirectionTargetAttribute ).toUrl();
    if ( redirectUrl.isValid() && url != redirectUrl )
    {
      delete reply;
      url = redirectUrl;
    }
    else
    {
      break;
    }
  }

  errMsg = reply->error() == QNetworkReply::NoError ? QString() : reply->errorString();
  delete reply;
  return errMsg.isEmpty();
}

/// MAP ///////////////////////////////////////////////////////////////////////

void KadasGlobeDialog::addImagery( const QString &type, const QString &uri )
{
  QTreeWidgetItem *item = new QTreeWidgetItem( QStringList() << type << uri );
  item->setFlags( item->flags() & ~Qt::ItemIsDropEnabled );
  mImageryTreeView->addTopLevelItem( item );
  mImageryTreeView->resizeColumnToContents( 0 );
}

void KadasGlobeDialog::addTMSImagery()
{
  addImagery( "TMS", qobject_cast<QAction *>( QObject::sender() )->data().toString() );
}

void KadasGlobeDialog::addCustomTMSImagery()
{
  QString url = QInputDialog::getText( this, tr( "Add TMS Imagery" ), tr( "TMS URL:" ) );
  if ( !url.isEmpty() )
  {
    QString validationError;
    if ( !validateRemoteUri( url, validationError ) )
    {
      QMessageBox::warning( this, tr( "Add TMS Imagery" ), validationError );
    }
    else
    {
      addImagery( "TMS", url );
    }
  }
}

void KadasGlobeDialog::addCustomWMSImagery()
{
  QString url = QInputDialog::getText( this, tr( "Add WMS Imagery" ), tr( "URL:" ) );
  if ( !url.isEmpty() )
  {
    QString validationError;
    if ( !validateRemoteUri( url, validationError ) )
    {
      QMessageBox::warning( this, tr( "Add WMS Imagery" ), validationError );
    }
    else
    {
      addImagery( "WMS", url );
    }
  }
}

void KadasGlobeDialog::addRasterImagery()
{
  addImagery( "Raster", qobject_cast<QAction *>( QObject::sender() )->data().toString() );
}

void KadasGlobeDialog::addCustomRasterImagery()
{
  QString filename = QFileDialog::getOpenFileName( this, tr( "Add Raster Imagery" ) );
  if ( !filename.isEmpty() )
  {
    addImagery( "Raster", filename );
  }
}

void KadasGlobeDialog::addElevation( const QString &type, const QString &uri )
{
  QTreeWidgetItem *item = new QTreeWidgetItem( QStringList() << type << uri );
  item->setFlags( item->flags() & ~Qt::ItemIsDropEnabled );
  mElevationTreeView->addTopLevelItem( item );
  mElevationTreeView->resizeColumnToContents( 0 );
}

void KadasGlobeDialog::addTMSElevation()
{
  addElevation( "TMS", qobject_cast<QAction *>( QObject::sender() )->data().toString() );
}

void KadasGlobeDialog::addCustomTMSElevation()
{
  QString url = QInputDialog::getText( this, tr( "Add TMS Elevation" ), tr( "TMS URL:" ) );
  if ( !url.isEmpty() )
  {
    QString validationError;
    if ( !validateRemoteUri( url, validationError ) )
    {
      QMessageBox::warning( this, tr( "Add TMS Elevation" ), validationError );
    }
    else
    {
      addElevation( "TMS", url );
    }
  }
}

void KadasGlobeDialog::addCustomRasterElevation()
{
  QString filename = QFileDialog::getOpenFileName( this, tr( "Add Raster Elevation" ) );
  if ( !filename.isEmpty() )
  {
    addElevation( "Raster", filename );
  }
}

void KadasGlobeDialog::on_mRemoveImageryButton_clicked()
{
  delete mImageryTreeView->currentItem();
}

void KadasGlobeDialog::on_mRemoveElevationButton_clicked()
{
  delete mElevationTreeView->currentItem();
}

QList<KadasGlobeDialog::LayerDataSource> KadasGlobeDialog::getImageryDataSources() const
{
  int keysCount = QgsProject::instance()->subkeyList( "Globe-Plugin", "/imageryDatasources/" ).count();
  QList<LayerDataSource> datasources;
  for ( int i = 0; i < keysCount; ++i )
  {
    QString key = QString( "/imageryDatasources/L%1" ).arg( i );
    LayerDataSource datasource;
    datasource.type  = QgsProject::instance()->readEntry( "Globe-Plugin", key + "/type" );
    datasource.uri   = QgsProject::instance()->readPath( QgsProject::instance()->readEntry( "Globe-Plugin", key + "/uri" ) );
    datasources.append( datasource );
  }
  return datasources;
}

QList<KadasGlobeDialog::LayerDataSource> KadasGlobeDialog::getElevationDataSources() const
{
  int keysCount = QgsProject::instance()->subkeyList( "Globe-Plugin", "/elevationDatasources/" ).count();
  QList<LayerDataSource> datasources;
  for ( int i = 0; i < keysCount; ++i )
  {
    QString key = QString( "/elevationDatasources/L%1" ).arg( i );
    LayerDataSource datasource;
    datasource.type  = QgsProject::instance()->readEntry( "Globe-Plugin", key + "/type" );
    datasource.uri   = QgsProject::instance()->readPath( QgsProject::instance()->readEntry( "Globe-Plugin", key + "/uri" ) );
    datasources.append( datasource );
  }
  return datasources;
}

double KadasGlobeDialog::getVerticalScale() const
{
  return mSpinBoxVerticalScale->value();
}

bool KadasGlobeDialog::getSkyEnabled() const
{
  return QgsProject::instance()->readBoolEntry( "Globe-Plugin", "/skyEnabled", true );
}

QDateTime KadasGlobeDialog::getSkyDateTime() const
{
  bool overrideDateTime = QgsProject::instance()->readBoolEntry( "Globe-Plugin", "/overrideDateTime", false );
  if ( overrideDateTime )
  {
    return QDateTime::fromString( QgsProject::instance()->readEntry( "Globe-Plugin", "/skyDateTime", QDateTime::currentDateTime().toString() ) );
  }
  else
  {
    return QDateTime::currentDateTime();
  }
}

bool KadasGlobeDialog::getSkyAutoAmbience() const
{
  return QgsProject::instance()->readBoolEntry( "Globe-Plugin", "/skyAutoAmbient", false );
}

double KadasGlobeDialog::getSkyMinAmbient() const
{
  return QgsProject::instance()->readDoubleEntry( "Globe-Plugin", "/skyMinAmbient", 30. ) / 100.;
}

/// ADVANCED //////////////////////////////////////////////////////////////////

float KadasGlobeDialog::getScrollSensitivity() const
{
  return sliderScrollSensitivity->value() / 10;
}

bool KadasGlobeDialog::getInvertScrollWheel() const
{
  return checkBoxInvertScroll->checkState();
}

/// STEREO ////////////////////////////////////////////////////////////////////
void KadasGlobeDialog::on_pushButtonStereoResetDefaults_clicked()
{
  //http://www.openscenegraph.org/projects/osg/wiki/Support/UserGuides/StereoSettings
  comboBoxStereoMode->setCurrentIndex( comboBoxStereoMode->findText( "OFF" ) );
  spinBoxStereoScreenDistance->setValue( 0.5 );
  spinBoxStereoScreenHeight->setValue( 0.26 );
  spinBoxStereoScreenWidth->setValue( 0.325 );
  spinBoxStereoEyeSeparation->setValue( 0.06 );
  spinBoxSplitStereoHorizontalSeparation->setValue( 42 );
  spinBoxSplitStereoVerticalSeparation->setValue( 42 );
  comboBoxSplitStereoHorizontalEyeMapping->setCurrentIndex( 0 );
  comboBoxSplitStereoVerticalEyeMapping->setCurrentIndex( 0 );
}

void KadasGlobeDialog::on_comboBoxStereoMode_currentIndexChanged( int index )
{
  //http://www.openscenegraph.org/projects/osg/wiki/Support/UserGuides/StereoSettings
  //http://www.openscenegraph.org/documentation/OpenSceneGraphReferenceDocs/a00181.html

  int stereoMode = comboBoxStereoMode->itemData( index ).toInt();
  bool stereoEnabled = stereoMode != -1;
  bool verticalSplit = stereoMode == osg::DisplaySettings::VERTICAL_SPLIT;
  bool horizontalSplit = stereoMode == osg::DisplaySettings::HORIZONTAL_SPLIT;

  spinBoxStereoScreenDistance->setEnabled( stereoEnabled );
  spinBoxStereoScreenHeight->setEnabled( stereoEnabled );
  spinBoxStereoScreenWidth->setEnabled( stereoEnabled );
  spinBoxStereoEyeSeparation->setEnabled( stereoEnabled );
  spinBoxSplitStereoHorizontalSeparation->setEnabled( stereoEnabled && horizontalSplit );
  comboBoxSplitStereoHorizontalEyeMapping->setEnabled( stereoEnabled && horizontalSplit );
  spinBoxSplitStereoVerticalSeparation->setEnabled( stereoEnabled && verticalSplit );
  comboBoxSplitStereoVerticalEyeMapping->setEnabled( stereoEnabled && verticalSplit );
}
