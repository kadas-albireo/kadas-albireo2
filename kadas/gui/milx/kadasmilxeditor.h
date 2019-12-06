/***************************************************************************
    kadasmilxeditor.h
    -----------------
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

#ifndef KADASMILXEDITOR_H
#define KADASMILXEDITOR_H

#include <kadas/gui/mapitemeditors/kadasmapitemeditor.h>
#include <kadas/gui/milx/kadasmilxclient.h>

class QToolButton;
class KadasMilxLibrary;


class KADAS_GUI_EXPORT KadasMilxEditor : public KadasMapItemEditor
{
  public:
    KadasMilxEditor( KadasMapItem *item, EditorType type, KadasMilxLibrary *library, QWidget *parent = nullptr );

  public slots:
    void syncItemToWidget() override;
    void syncWidgetToItem() override;

  private slots:
    void toggleLibrary( bool enabled );
    void symbolSelected( const KadasMilxSymbolDesc &symbolTemplate );

  private:
    KadasMilxLibrary *mLibrary = nullptr;
    QToolButton *mSymbolButton = nullptr;
    KadasMilxSymbolDesc mSelectedSymbol;

};

#endif // KADASMILXEDITOR_H
