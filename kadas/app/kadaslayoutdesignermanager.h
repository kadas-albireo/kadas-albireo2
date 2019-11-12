/***************************************************************************
    kadaslayoutdesignermanager.h
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

#ifndef KADASLAYOUTDESIGNERMANAGER_H
#define KADASLAYOUTDESIGNERMANAGER_H

#include <QMap>
#include <QObject>

class QgsPrintLayout;
class KadasLayoutDesignerDialog;


class KadasLayoutDesignerManager : public QObject
{
    Q_OBJECT
  public:
    static KadasLayoutDesignerManager *instance();

    void openDesigner( QgsPrintLayout *layout );
    void closeAllDesigners();

  private:
    KadasLayoutDesignerManager() = default;

    QList<QObject *> mOpenDialogs;
};

#endif // KADASLAYOUTDESIGNERMANAGER_H
