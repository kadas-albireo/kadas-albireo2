/***************************************************************************
    kadasmapitemtooltip.h
    ---------------------
    copyright            : (C) 2021 by Sandro Mani
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

#ifndef KADASMAPITEMTOOLTIP_H
#define KADASMAPITEMTOOLTIP_H

#include <QTextBrowser>
#include <QTimer>

#include "kadas/gui/kadas_gui.h"

class QgsMapCanvas;
class KadasMapItem;

class KADAS_GUI_EXPORT KadasMapItemTooltip : public QTextEdit {
  Q_OBJECT
public:
  KadasMapItemTooltip(QgsMapCanvas *canvas);
  void updateForPos(const QPoint &canvasPos);
  QVariant loadResource(int type, const QUrl &url) override;

public slots:
  void clear();

protected:
  void enterEvent(QEvent *) override;
  void mousePressEvent(QMouseEvent *ev) override;
  void mouseMoveEvent(QMouseEvent *ev) override;
  void mouseReleaseEvent(QMouseEvent *ev) override;

private:
  static constexpr int sWidth = 320;
  static constexpr int sHeight = 240;

  QTimer mShowTimer;
  QTimer mHideTimer;
  QPoint mPos;
  QgsMapCanvas *mCanvas = nullptr;
  bool mMouseMoved = false;
  const KadasMapItem *mItem = nullptr;

private slots:
  void positionAndShow();
};

#endif // KADASMAPITEMTOOLTIP_H
