/***************************************************************************
    kadasmilxintegration.cpp
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

#include <QAction>
#include <QDialog>
#include <QFileDialog>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QSlider>
#include <QTabWidget>
#include <quazip5/quazipfile.h>

#include <qgis/qgslogger.h>
#include <qgis/qgsmessagebar.h>
#include <qgis/qgsproject.h>
#include <qgis/qgssettings.h>

#include <kadas/gui/maptools/kadasmaptoolcreateitem.h>
#include <kadas/gui/milx/kadasmilxclient.h>
#include <kadas/gui/milx/kadasmilxeditor.h>
#include <kadas/gui/milx/kadasmilxitem.h>
#include <kadas/gui/milx/kadasmilxlayer.h>
#include <kadas/gui/milx/kadasmilxlayerpropertiespage.h>
#include <kadas/gui/milx/kadasmilxlibrary.h>
#include <kadas/app/kadasapplication.h>
#include <kadas/app/kadasmainwindow.h>
#include <kadas/app/milx/kadasmilxintegration.h>
#include <kadas/app/ui_KadasMilxExportDialog.h>

KadasMilxIntegration::KadasMilxIntegration( const MilxUi &ui, QObject *parent )
  : QObject( parent ), mUi( ui ), mMilxLibrary( nullptr )
{
  if ( !KadasMilxClient::init() )
  {
    QgsDebugMsg( "Failed to initialize the MilX library." );
    return;
  }

  connect( mUi.mActionMilx, &QAction::triggered, this, &KadasMilxIntegration::createMilx );
  connect( mUi.mActionSaveMilx, &QAction::triggered, this, &KadasMilxIntegration::saveMilxly );
  connect( mUi.mActionLoadMilx, &QAction::triggered, this, &KadasMilxIntegration::openMilxly );
  connect( QgsProject::instance(), &QgsProject::readProject, this, &KadasMilxIntegration::readProjectSettings );

  mUi.mRibbonWidget->setTabEnabled( mUi.mRibbonWidget->indexOf( mUi.mMssTab ), true );

  mUi.mSymbolSizeSlider->setRange( KadasMilxSymbolSettings::MinSymbolSize, KadasMilxSymbolSettings::MaxSymbolSize );
  mUi.mSymbolSizeSlider->setValue( QgsProject::instance()->readNumEntry( "milx", "symbol_size", KadasMilxSymbolSettings::DefaultSymbolSize ) );
  setMilXSymbolSize( mUi.mSymbolSizeSlider->value() );
  connect( mUi.mSymbolSizeSlider, &QSlider::valueChanged, this, &KadasMilxIntegration::setMilXSymbolSize );

  mUi.mLineWidthSlider->setRange( KadasMilxSymbolSettings::MinLineWidth, KadasMilxSymbolSettings::MaxLineWidth );
  mUi.mLineWidthSlider->setValue( QgsProject::instance()->readNumEntry( "milx", "line_width", KadasMilxSymbolSettings::DefaultLineWidth ) );
  setMilXLineWidth( mUi.mLineWidthSlider->value() );
  connect( mUi.mLineWidthSlider, &QSlider::valueChanged, this, &KadasMilxIntegration::setMilXLineWidth );

  mUi.mWorkModeCombo->addItem( tr( "International" ), KadasMilxSymbolSettings::WorkModeInternational );
  mUi.mWorkModeCombo->addItem( tr( "CH" ), KadasMilxSymbolSettings::WorkModeCH );
  mUi.mWorkModeCombo->setCurrentIndex( QgsProject::instance()->readNumEntry( "milx", "work_mode", KadasMilxSymbolSettings::DefaultWorkMode ) );
  setMilXWorkMode( mUi.mWorkModeCombo->currentIndex() );
  connect( mUi.mWorkModeCombo, qOverload<int>( &QComboBox::currentIndexChanged ), this, &KadasMilxIntegration::setMilXWorkMode );

  mMilxLibrary = new KadasMilxLibrary( kApp->mainWindow()->winId() );

  KadasMapItemEditor::registry()->insert( "KadasMilxEditor", [this]( KadasMapItem * item, KadasMapItemEditor::EditorType type )
  {
    return new KadasMilxEditor( item, type, mMilxLibrary );
  } );

  kApp->mainWindow()->addCustomDropHandler( &mDropHandler );

  mLayerPropertiesFactory = new KadasMilxLayerPropertiesPageFactory( this );
  kApp->registerMapLayerPropertiesFactory( mLayerPropertiesFactory );
}

KadasMilxIntegration::~KadasMilxIntegration()
{
  kApp->mainWindow()->removeCustomDropHandler( &mDropHandler );
  KadasMilxClient::quit();
  delete mMilxLibrary;
}

KadasMilxLayer *KadasMilxIntegration::getLayer()
{
  for ( QgsMapLayer *layer : QgsProject::instance()->mapLayers() )
  {
    if ( dynamic_cast<KadasMilxLayer *>( layer ) )
    {
      return static_cast<KadasMilxLayer *>( layer );
    }
  }
  return nullptr;
}

KadasMilxLayer *KadasMilxIntegration::getOrCreateLayer()
{
  KadasMilxLayer *layer = getLayer();
  if ( !layer )
  {
    layer = new KadasMilxLayer();
    QgsProject::instance()->addMapLayer( layer );
  }
  return layer;
}

void KadasMilxIntegration::createMilx( bool active )
{
  QgsMapCanvas *canvas = kApp->mainWindow()->mapCanvas();
  if ( active )
  {

    KadasMapToolCreateItem::ItemFactory itemFactory = [ = ]
    {
      return new KadasMilxItem();
    };
    KadasLayerSelectionWidget::LayerFilter layerFilter = [ = ]( QgsMapLayer * layer ) { return dynamic_cast<KadasMilxLayer *>( layer ); };
    KadasLayerSelectionWidget::LayerCreator layerCreator = [ = ]( const QString & name ) { return new KadasMilxLayer( name ); };

    KadasMapToolCreateItem *tool = new KadasMapToolCreateItem( canvas, itemFactory, getOrCreateLayer() );
    tool->setAction( mUi.mActionMilx );
    tool->showLayerSelection( true, kApp->mainWindow()->layerTreeView(), layerFilter, layerCreator );
    kApp->mainWindow()->layerTreeView()->setCurrentLayer( getOrCreateLayer() );
    kApp->mainWindow()->layerTreeView()->setLayerVisible( getOrCreateLayer(), true );
    canvas->setMapTool( tool );
  }
  else if ( !active && canvas->mapTool() && canvas->mapTool()->action() == mUi.mActionMilx )
  {
    canvas->unsetMapTool( canvas->mapTool() );
  }
}

void KadasMilxIntegration::readProjectSettings()
{
  mUi.mSymbolSizeSlider->setValue( QgsProject::instance()->readNumEntry( "milx", "symbol_size", 60 ) );
  mUi.mLineWidthSlider->setValue( QgsProject::instance()->readNumEntry( "milx", "line_width", 2 ) );
  mUi.mWorkModeCombo->setCurrentIndex( QgsProject::instance()->readNumEntry( "milx", "work_mode", 1 ) );
}

void KadasMilxIntegration::setMilXSymbolSize( int value )
{
  QgsProject::instance()->writeEntry( "milx", "symbol_size", value );
  KadasMilxClient::setSymbolSize( value );
  refreshMilxLayers();
}

void KadasMilxIntegration::setMilXLineWidth( int value )
{
  QgsProject::instance()->writeEntry( "milx", "line_width", value );
  KadasMilxClient::setLineWidth( value );
  refreshMilxLayers();
}

void KadasMilxIntegration::setMilXWorkMode( int idx )
{
  QgsProject::instance()->writeEntry( "milx", "work_mode", idx );
  KadasMilxClient::setWorkMode( static_cast<KadasMilxSymbolSettings::WorkMode>( idx ) );
  refreshMilxLayers();
}

void KadasMilxIntegration::refreshMilxLayers()
{
  for ( QgsMapLayer *layer : QgsProject::instance()->mapLayers().values() )
  {
    if ( qobject_cast<KadasMilxLayer *>( layer ) )
    {
      layer->triggerRepaint();
    }
  }
}

void KadasMilxIntegration::saveMilxly()
{
  kApp->unsetMapTool();

  QDialog exportDialog;
  Ui::KadasMilxExportDialog ui;
  ui.setupUi( &exportDialog );

  ui.comboCartouche->addItem( tr( "Don't add" ) );
  for ( QgsMapLayer *layer : QgsProject::instance()->mapLayers().values() )
  {
    if ( qobject_cast<KadasMilxLayer *>( layer ) )
    {
      QListWidgetItem *item = new QListWidgetItem( layer->name() );
      item->setData( Qt::UserRole, layer->id() );
      item->setCheckState( kApp->mainWindow()->mapCanvas()->layers().contains( layer ) ? Qt::Checked : Qt::Unchecked );
      ui.listWidget->addItem( item );
      ui.comboCartouche->addItem( layer->name(), layer->id() );
    }
  }
  QStringList versionTags, versionNames;
  KadasMilxClient::getSupportedLibraryVersionTags( versionTags, versionNames );
  for ( int i = 0, n = versionTags.size(); i < n; ++i )
  {
    ui.comboMilxVersion->addItem( versionNames[i], versionTags[i] );
  }
  ui.comboMilxVersion->setCurrentIndex( 0 );

  if ( exportDialog.exec() == QDialog::Rejected )
  {
    return;
  }

  QStringList exportLayers;
  for ( int i = 0, n = ui.listWidget->count(); i < n; ++i )
  {
    QListWidgetItem *item = ui.listWidget->item( i );
    if ( item->checkState() == Qt::Checked )
    {
      exportLayers.append( item->data( Qt::UserRole ).toString() );
    }
  }

  QStringList filters;
  filters.append( tr( "Compressed MilX Layer (*.milxlyz)" ) );
  filters.append( tr( "MilX Layer (*.milxly)" ) );

  QString lastDir = QgsSettings().value( "/UI/lastImportExportDir", "." ).toString();
  QString selectedFilter;

  QString filename = QFileDialog::getSaveFileName( 0, tr( "Select Output" ), lastDir, filters.join( ";;" ), &selectedFilter );
  if ( filename.isEmpty() )
  {
    return;
  }
  QgsSettings().setValue( "/UI/lastImportExportDir", QFileInfo( filename ).absolutePath() );
  if ( selectedFilter == filters[0] && !filename.endsWith( ".milxlyz", Qt::CaseInsensitive ) )
  {
    filename += ".milxlyz";
  }
  else if ( selectedFilter == filters[1] && !filename.endsWith( ".milxly", Qt::CaseInsensitive ) )
  {
    filename += ".milxly";
  }
  QString targetVersionTag = ui.comboMilxVersion->currentData().toString();
  QString currentVersionTag; KadasMilxClient::getCurrentLibraryVersionTag( currentVersionTag );

  QIODevice *dev = 0;
  QuaZip *zip = 0;
  if ( selectedFilter == filters[0] )
  {
    zip = new QuaZip( filename );
    zip->open( QuaZip::mdCreate );
    dev = new QuaZipFile( zip );
    QuaZipNewInfo info( "Layer.milxly" );
    info.setPermissions( QFile::ReadOwner | QFile::ReadUser | QFile::ReadGroup | QFile::ReadOther );
    static_cast<QuaZipFile *>( dev )->open( QIODevice::WriteOnly, info );
  }
  else
  {
    dev = new QFile( filename );
    dev->open( QIODevice::WriteOnly );
  }
  if ( !dev->isOpen() )
  {
    delete dev;
    delete zip;
    kApp->mainWindow()->messageBar()->pushMessage( tr( "Export Failed" ), tr( "Failed to open the output file for writing." ), Qgis::Critical, 5 );
    return;
  }

  int dpi = kApp->mainWindow()->mapCanvas()->mapSettings().outputDpi();

  QDomDocument doc;
  doc.appendChild( doc.createProcessingInstruction( "xml", "version=\"1.0\" encoding=\"UTF-8\"" ) );
  QDomElement milxDocumentEl = doc.createElement( "MilXDocument_Layer" );
  milxDocumentEl.setAttribute( "xmlns", "http://gs-soft.com/MilX/V3.1" );
  milxDocumentEl.setAttribute( "xmlns:xsi", "http://www.w3.org/2001/XMLSchema-instance" );
  doc.appendChild( milxDocumentEl );

  QDomElement milxVersionEl = doc.createElement( "MssLibraryVersionTag" );
  milxVersionEl.appendChild( doc.createTextNode( currentVersionTag ) );
  milxDocumentEl.appendChild( milxVersionEl );

  QString cartoucheLayerId = ui.comboCartouche->currentData().toString();
  QString cartouche = QgsProject::instance()->readEntry( "VBS-Print", "cartouche" );

  // Replace custom texts by "Custom" since the MSS Schema Validator enforces this
  QStringList validClassifications = QStringList() << "None" << "Internal" << "Confidential" << "Secret";
  QDomDocument cartoucheDoc;
  cartoucheDoc.setContent( cartouche );
  QDomNodeList exClassifications = cartoucheDoc.elementsByTagName( "ExerciseClassification" );
  if ( !exClassifications.isEmpty() )
  {
    QDomElement el = exClassifications.at( 0 ).toElement();
    QString classification = el.text();
    if ( !validClassifications.contains( classification ) )
      el.replaceChild( cartoucheDoc.createTextNode( "Custom" ), el.firstChild() );
  }

  QDomNodeList missingClassifications = cartoucheDoc.elementsByTagName( "MissionClassification" );
  if ( !missingClassifications.isEmpty() )
  {
    QDomElement el = missingClassifications.at( 0 ).toElement();
    QString classification = el.text();
    if ( !validClassifications.contains( classification ) )
      el.replaceChild( cartoucheDoc.createTextNode( "Custom" ), el.firstChild() );
  }
  cartouche = cartoucheDoc.toString();

  for ( const QString &layerId : exportLayers )
  {
    QgsMapLayer *layer = QgsProject::instance()->mapLayer( layerId );
    if ( qobject_cast<KadasMilxLayer *>( layer ) )
    {
      QDomElement milxLayerEl = doc.createElement( "MilXLayer" );
      milxDocumentEl.appendChild( milxLayerEl );
      static_cast<KadasMilxLayer *>( layer )->exportToMilxly( milxLayerEl, dpi );

      if ( cartoucheLayerId == layerId )
      {
        QDomDocument cartoucheDoc;
        cartoucheDoc.setContent( cartouche );
        milxLayerEl.appendChild( cartoucheDoc.documentElement() );
      }
    }
  }
  QString inputXml = doc.toString();
  QString outputXml;
  bool valid = false;
  QString messages;
  if ( !KadasMilxClient::downgradeMilXFile( inputXml, outputXml, targetVersionTag, valid, messages ) )
  {
    delete dev;
    delete zip;
    kApp->mainWindow()->messageBar()->pushMessage( tr( "MilX export failed" ), tr( "Failed to write output." ), Qgis::Critical, 5 );
    return;
  }
  if ( valid )
  {
    dev->write( outputXml.toUtf8() );
    kApp->mainWindow()->messageBar()->pushMessage( tr( "MilX export completed" ), "", Qgis::Info, 5 );
    if ( !messages.isEmpty() )
    {
      showMessageDialog( tr( "Export Messages" ), tr( "The following messages were emitted while exporting:" ), messages );
    }
  }
  else
  {
    kApp->mainWindow()->messageBar()->pushMessage( tr( "Export Failed" ), "", Qgis::Critical, 5 );
    if ( !messages.isEmpty() )
    {
      showMessageDialog( tr( "Export Failed" ), tr( "The export failed:" ), messages );
    }
  }

  delete dev;
  delete zip;
}

void KadasMilxIntegration::openMilxly()
{
  QString lastDir = QgsSettings().value( "/UI/lastImportExportDir", "." ).toString();
  QString filter = tr( "MilX Layer Files (*.milxly *.milxlyz)" );
  QString filename = QFileDialog::getOpenFileName( 0, tr( "Select Milx Layer File" ), lastDir, filter );
  if ( filename.isEmpty() )
  {
    return;
  }
  QgsSettings().setValue( "/UI/lastImportExportDir", QFileInfo( filename ).absolutePath() );

  QString errorMsg;
  if ( !importMilxly( filename, errorMsg ) )
  {
    kApp->mainWindow()->messageBar()->pushMessage( tr( "MilX import failed" ), errorMsg, Qgis::Critical, 5 );
  }
  else
  {
    kApp->mainWindow()->messageBar()->pushMessage( tr( "MilX import completed" ), "", Qgis::Info, 5 );
  }
}

bool KadasMilxIntegration::importMilxly( const QString &filename, QString &errorMsg )
{
  QIODevice *dev = 0;
  if ( filename.endsWith( ".milxlyz", Qt::CaseInsensitive ) )
  {
    dev = new QuaZipFile( filename, "Layer.milxly", QuaZip::csInsensitive );
  }
  else
  {
    dev = new QFile( filename );
  }
  if ( !dev->open( QIODevice::ReadOnly ) )
  {
    delete dev;
    errorMsg = tr( "Failed to open the input file." );
    return false;
  }
  QString inputXml = QString::fromUtf8( dev->readAll().data() );
  delete dev;
  QString outputXml;
  bool valid = false;
  QString messages;
  if ( !KadasMilxClient::upgradeMilXFile( inputXml, outputXml, valid, messages ) )
  {
    errorMsg = tr( "Failed to read input file." );
    return false;
  }
  if ( !valid )
  {
    if ( !messages.isEmpty() )
    {
      showMessageDialog( tr( "Import Failed" ), tr( "The import failed:" ), messages );
    }
    errorMsg = tr( "MilX upgrade failed" );
    return false;
  }

  QDomDocument doc;
  doc.setContent( outputXml );

  if ( doc.isNull() )
  {
    errorMsg = tr( "The file could not be parsed." );
    return false;
  }

  QDomElement milxDocumentEl = doc.firstChildElement( "MilXDocument_Layer" );
  QDomElement milxVersionEl = milxDocumentEl.firstChildElement( "MssLibraryVersionTag" );
  QString fileMssVer = milxVersionEl.text();

  QString verTag;
  KadasMilxClient::getCurrentLibraryVersionTag( verTag );
  if ( fileMssVer != verTag )
  {
    errorMsg = tr( "Unexpected MSS library version tag." );
    return false;
  }
  int dpi = kApp->mainWindow()->mapCanvas()->mapSettings().outputDpi();

  QDomNodeList milxLayerEls = milxDocumentEl.elementsByTagName( "MilXLayer" );
  QList<KadasMilxLayer *> importedLayers;
  QList< QPair<QString, QString> > cartouches;
  for ( int iLayer = 0, nLayers = milxLayerEls.count(); iLayer < nLayers; ++iLayer )
  {
    QDomElement milxLayerEl = milxLayerEls.at( iLayer ).toElement();
    KadasMilxLayer *layer = new KadasMilxLayer();
    if ( !layer->importFromMilxly( milxLayerEl, dpi, errorMsg ) )
    {
      break;
    }
    importedLayers.append( layer );
    QDomNodeList cartoucheEls = milxLayerEl.elementsByTagName( "Legend" );
    if ( !cartoucheEls.isEmpty() )
    {
      QString cartouche;
      QTextStream ts( &cartouche );
      cartoucheEls.at( 0 ).save( ts, 2 );
      cartouches.append( qMakePair( layer->name(), cartouche ) );
    }
  }

  if ( errorMsg.isEmpty() )
  {
    for ( KadasMilxLayer *layer : importedLayers )
    {
      QgsProject::instance()->addMapLayer( layer );
    }
    if ( !cartouches.isEmpty() )
    {
      QDialog cartoucheSelectionDialog( kApp->mainWindow() );
      cartoucheSelectionDialog.setWindowTitle( tr( "Import cartouche" ) );
      QGridLayout *layout = new QGridLayout();
      cartoucheSelectionDialog.setLayout( layout );
      layout->addWidget( new QLabel( tr( "Import cartouche from MilX layer:" ) ), 0, 0, 1, 1 );
      QComboBox *cartoucheCombo = new QComboBox();
      cartoucheCombo->addItem( tr( "Don't import" ) );
      typedef QPair<QString, QString> CartouchePair;
      for ( const CartouchePair &pair : cartouches )
      {
        cartoucheCombo->addItem( pair.first, pair.second );
      }
      layout->addWidget( cartoucheCombo, 0, 1, 1, 1 );
      QDialogButtonBox *bbox = new QDialogButtonBox( QDialogButtonBox::Ok );
      connect( bbox, &QDialogButtonBox::accepted, &cartoucheSelectionDialog, &QDialog::accept );
      connect( bbox, &QDialogButtonBox::rejected, &cartoucheSelectionDialog, &QDialog::reject );
      layout->addWidget( bbox, 1, 1, 1, 2 );
      if ( cartoucheSelectionDialog.exec() == QDialog::Accepted )
      {
        QString cartouche = cartoucheCombo->itemData( cartoucheCombo->currentIndex() ).toString();
        if ( !cartouche.isEmpty() )
        {
          QgsProject::instance()->writeEntry( "VBS-Print", "cartouche", cartouche );
        }
      }
    }

    if ( !messages.isEmpty() )
    {
      showMessageDialog( tr( "Import Messages" ), tr( "The following messages were emitted while importing:" ), messages );
    }
  }
  else
  {
    qDeleteAll( importedLayers );
  }
  return true;
}

void KadasMilxIntegration::showMessageDialog( const QString &title, const QString &body, const QString &messages )
{
  QDialog dialog;
  dialog.setWindowTitle( title );
  dialog.setLayout( new QVBoxLayout );
  dialog.layout()->addWidget( new QLabel( body ) );
  QPlainTextEdit *textEdit = new QPlainTextEdit( messages );
  textEdit->setReadOnly( true );
  dialog.layout()->addWidget( textEdit );
  QDialogButtonBox *bbox = new QDialogButtonBox( QDialogButtonBox::Close );
  dialog.layout()->addWidget( bbox );
  connect( bbox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept );
  connect( bbox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject );
  dialog.exec();
}

bool KadasMilxDropHandler::canHandleMimeData( const QMimeData *data )
{
  for ( const QUrl &url : data->urls() )
  {
    if ( url.toLocalFile().endsWith( ".milxly", Qt::CaseInsensitive ) || url.toLocalFile().endsWith( ".milxlyz", Qt::CaseInsensitive ) )
    {
      return true;
    }
  }
  return false;
}

bool KadasMilxDropHandler::handleMimeDataV2( const QMimeData *data )
{
  int handled = 0;
  for ( const QUrl &url : data->urls() )
  {
    QString path = url.toLocalFile();
    QStringList errors;
    if ( path.endsWith( ".milxly", Qt::CaseInsensitive ) || path.endsWith( ".milxlyz", Qt::CaseInsensitive ) )
    {
      ++handled;
      QString errMsg;
      if ( !KadasMilxIntegration::importMilxly( path, errMsg ) )
      {
        errors.append( QString( "%1: %2" ).arg( QFileInfo( path ).fileName() ).arg( errMsg ) );
      }
    }
    if ( handled > 0 )
    {
      if ( errors.isEmpty() )
      {
        kApp->mainWindow()->messageBar()->pushMessage( tr( "MilX import completed" ), Qgis::Info, 5 );
      }
      else
      {
        QMessageBox::critical( kApp->mainWindow(), tr( "MilX import failed" ), tr( "The following files could not be imported:\n%1" ).arg( errors.join( "\n" ) ) );
      }
    }
  }
  return handled > 0;
}
