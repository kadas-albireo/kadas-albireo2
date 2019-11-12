/***************************************************************************
    kadaslayerpropertiesdialog.h
    ----------------------------
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

#ifndef KADASLAYERPROPERTIESDIALOG_H
#define KADASLAYERPROPERTIESDIALOG_H

#include <QDialog>

#include <kadas/gui/kadas_gui.h>
#include <kadas/gui/ui_kadaslayerpropertiesdialog.h>

class QgsMapLayerConfigWidget;
class QgsMapLayerConfigWidgetFactory;
class QgsMapLayer;


class KADAS_GUI_EXPORT KadasLayerPropertiesDialog : public QDialog, protected Ui::KadasLayerPropertiesBase
{
    Q_OBJECT
  public:
    KadasLayerPropertiesDialog( QgsMapLayer *layer, QWidget *parent = nullptr );
    void addPropertiesPageFactory( QgsMapLayerConfigWidgetFactory *factory );

  protected slots:
    virtual void apply();

  private:
    QgsMapLayer *mLayer = nullptr;
    QList<QgsMapLayerConfigWidget *> mLayerPropertiesPages;
};

#endif // KADASLAYERPROPERTIESDIALOG_H
