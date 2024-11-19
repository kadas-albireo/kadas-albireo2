/***************************************************************************
    kadasribbonbutton.h
    -------------------
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

#ifndef KADASRIBBONBUTTON_H
#define KADASRIBBONBUTTON_H

#include <QToolButton>

#include "kadas/gui/kadas_gui.h"

class KADAS_GUI_EXPORT KadasRibbonButton : public QToolButton
{
    Q_OBJECT

  public:
    KadasRibbonButton( QWidget *parent = 0 )
      : QToolButton( parent ) {}

  signals:
    void contextMenuRequested( QPoint pos );

  protected:
    virtual void paintEvent( QPaintEvent *e ) override;
    virtual void enterEvent( QEvent *event ) override;
    virtual void leaveEvent( QEvent *event ) override;
    virtual void focusInEvent( QFocusEvent *event ) override;
    virtual void focusOutEvent( QFocusEvent *event ) override;
    virtual void mouseMoveEvent( QMouseEvent *event ) override;
    virtual void mousePressEvent( QMouseEvent *event ) override;
    virtual void contextMenuEvent( QContextMenuEvent *event ) override;
};

#endif // KADASRIBBONBUTTON_H
