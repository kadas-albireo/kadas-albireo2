/***************************************************************************
    kadascoordinateinput.h
    ----------------------
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

#ifndef KADASCOORDINATEINPUT_H
#define KADASCOORDINATEINPUT_H

#include <QWidget>

#include <qgis/qgscoordinatereferencesystem.h>
#include <qgis/qgspoint.h>

#include "kadas/gui/kadas_gui.h"

class QComboBox;
class QLineEdit;


class KADAS_GUI_EXPORT KadasCoordinateInput : public QWidget
{
    Q_OBJECT

  public:
    KadasCoordinateInput( QWidget *parent );

    const QgsPointXY &getCoordinate() const { return mCoo; }
    const QgsCoordinateReferenceSystem &getCrs() const { return mCrs; }
    bool isEmpty() const { return mEmpty; }

    void setCoordinate( const QgsPointXY &coo, const QgsCoordinateReferenceSystem &crs );

  private:
    static const int sFormatRole = Qt::UserRole + 1;
    static const int sAuthidRole = Qt::UserRole + 2;
    QgsPointXY mCoo;
    QgsCoordinateReferenceSystem mCrs;

    bool mEmpty = true;
    QLineEdit *mLineEdit;
    QComboBox *mCrsCombo;

  signals:
    void coordinateEdited();
    void coordinateChanged();

  private slots:
    void entryChanged();
    void entryEdited();
    void crsChanged();
};

#endif // KADASCOORDINATEINPUT_H
