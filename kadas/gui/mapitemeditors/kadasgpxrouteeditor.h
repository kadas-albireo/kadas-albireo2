/***************************************************************************
    kadasgpxrouteeditor.h
    ---------------------
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

#ifndef KADASGPXROUTEEDITOR_H
#define KADASGPXROUTEEDITOR_H

#include "kadas/gui/kadas_gui.h"
#include "kadas/gui/mapitemeditors/kadasmapitemeditor.h"
#include "kadas/gui/mapitems/kadasmapitem.h"
#include "kadas/gui/ui_kadasgpxrouteeditor.h"

class QgsSettingsEntryInteger;
class QgsSettingsEntryColor;
class QgsSettingsEntryString;
class QgsSettingsEntryColor;

class KADAS_GUI_EXPORT KadasGpxRouteEditor : public KadasMapItemEditor {
  Q_OBJECT

public:
  static const QgsSettingsEntryInteger *settingsGpxRouteSize;
  static const QgsSettingsEntryColor *settingsGpxRouteColor;
  static const QgsSettingsEntryString *settingsGpxRouteLabelFont;
  static const QgsSettingsEntryColor *settingsGpxRouteLabelColor;

  KadasGpxRouteEditor(KadasMapItem *item);
  ~KadasGpxRouteEditor();

  void setItem(KadasMapItem *item) override;
  void syncItemToWidget() override;
  void syncWidgetToItem() override;

private:
  Ui::KadasGpxRouteEditorBase mUi;

  QFont currentFont() const;

signals:
  void styleChanged();

private slots:
  void saveColor();
  void saveSize();
  void toggleItemMeasurements(bool enabled);
  void saveLabelFont();
  void saveLabelColor();
  void fontSizeChanged(int size);
};

#endif // KADASGPXROUTEEDITOR_H
