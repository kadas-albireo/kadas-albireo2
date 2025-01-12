#ifndef KADASCATALOGBROWSERPROPERTIESDIALOG_H
#define KADASCATALOGBROWSERPROPERTIESDIALOG_H

#include <QDialog>
#include <QStandardItem>

#include "kadas/gui/kadas_gui.h"
#include "kadas/gui/ui_kadascatalogbrowserpropertiesdialog.h"

class KADAS_GUI_EXPORT KadasCatalogBrowserPropertiesDialog : public QDialog, protected Ui::KadasCatalogBrowserPropertiesDialog
{
    Q_OBJECT

  public:
    explicit KadasCatalogBrowserPropertiesDialog( const QString &text, const QString &uri, const QString &metaDataUrl, const QString &subLayers, int sortIndex, QMimeData *mimeData, QWidget *parent = nullptr );
    ~KadasCatalogBrowserPropertiesDialog();
};

#endif // KADASCATALOGBROWSERPROPERTIESDIALOG_H
