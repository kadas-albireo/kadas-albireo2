
#include <QMimeData>

#include "kadas/gui/kadascatalogbrowserpropertiesdialog.h"


KadasCatalogBrowserPropertiesDialog::KadasCatalogBrowserPropertiesDialog( const QString &text, const QString &uri, const QString &metaDataUrl, const QString &subLayers, int sortIndex, QMimeData *mimeData, QWidget *parent )
  : QDialog( parent )
{
  setupUi( this );

  mQLabelText->setText( text );
  mQLabelUri->setText( uri );
  mQLabelMetaDataUrl->setText( metaDataUrl );
  mQLabelSubLayers->setText( subLayers );
  mQLabelSortIndex->setText( QString::number( sortIndex ) );

  if ( mimeData )
    mQLabelMimeData->setText( mimeData->text() );
  else
    mQLabelMimeData->setText( tr( "No Mime data for this item" ) );
}

KadasCatalogBrowserPropertiesDialog::~KadasCatalogBrowserPropertiesDialog()
{
}
