/***************************************************************************
    kadascoordinateformat.h
    -----------------------
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

#ifndef KADASCOORDINATEFORMAT_H
#define KADASCOORDINATEFORMAT_H

#include <QObject>

#include <qgis/qgsunittypes.h>

#include <kadas/core/kadas_core.h>

class QgsPointXY;
class QgsCoordinateReferenceSystem;

class KADAS_CORE_EXPORT KadasCoordinateFormat : public QObject
{
    Q_OBJECT
  public:
    enum Format
    {
      Default,
      DegMinSec,
      DegMin,
      DecDeg,
      UTM,
      MGRS
    };

    static KadasCoordinateFormat *instance();
    void getCoordinateDisplayFormat( Format &format, QString &epsg ) const;
    QgsUnitTypes::DistanceUnit getHeightDisplayUnit() const { return mHeightUnit; }

    QString getDisplayString( const QgsPointXY &p, const QgsCoordinateReferenceSystem &sSrs ) const;
    static QString getDisplayString( const QgsPointXY &p, const QgsCoordinateReferenceSystem &sSrs, Format format, const QString &epsg );

    double getHeightAtPos( const QgsPointXY &p, const QgsCoordinateReferenceSystem &crs, QString *errMsg = 0 );
    static double getHeightAtPos( const QgsPointXY &p, const QgsCoordinateReferenceSystem &crs, QgsUnitTypes::DistanceUnit unit, QString *errMsg = 0 );

    QgsPointXY parseCoordinate( const QString &text, Format format, bool &valid ) const;

  public slots:
    void setCoordinateDisplayFormat( Format format, const QString &epsg );
    void setHeightDisplayUnit( QgsUnitTypes::DistanceUnit heightUnit );

  signals:
    void coordinateDisplayFormatChanged( Format format, const QString &epsg );
    void heightDisplayUnitChanged( QgsUnitTypes::DistanceUnit heightUnit );

  private:
    KadasCoordinateFormat();

    Format mFormat;
    QString mEpsg;
    QgsUnitTypes::DistanceUnit mHeightUnit;
};

#endif // KADASCOORDINATEFORMAT_H
