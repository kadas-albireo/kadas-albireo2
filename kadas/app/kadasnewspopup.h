/***************************************************************************
    kadasnewspopup.h
    ----------------
    copyright            : (C) 2022 by Sandro Mani
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

#ifndef KADASNEWSPOPUP_H
#define KADASNEWSPOPUP_H

#include <QDialog>

class KadasNewsPopup : public QDialog {
  Q_OBJECT

public:
  static bool isConfigured();
  static void showIfNewsAvailable(bool force = false);

private:
  KadasNewsPopup(const QString &url);
  ~KadasNewsPopup();
  static KadasNewsPopup *sInstance;
};

#endif // KADASNEWSPOPUP_H
