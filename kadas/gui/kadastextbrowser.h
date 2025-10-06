/***************************************************************************
    kadastextbrowser.h
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

#ifndef KADASTEXTBROWSER_H
#define KADASTEXTBROWSER_H

#include "kadas/gui/kadas_gui.h"

#include <QTextBrowser>

// TODO: sip generates wrong code, errors out with sipNameNr_XXXX was not
// declared.
#ifndef SIP_RUN

class KADAS_GUI_EXPORT KadasTextBrowser : public QTextBrowser {
  Q_OBJECT

public:
  KadasTextBrowser(QWidget *parent = nullptr);

protected:
  void contextMenuEvent(QContextMenuEvent *e) override;
  void keyReleaseEvent(QKeyEvent *e) override;
  void insertFromMimeData(const QMimeData *source) override;
  void mousePressEvent(QMouseEvent *ev) override;
  void mouseReleaseEvent(QMouseEvent *ev) override;

private:
  const QRegExp &urlRegEx() const;
};

#endif // SIP_RUN

#endif // KADASTEXTBROWSER_H
