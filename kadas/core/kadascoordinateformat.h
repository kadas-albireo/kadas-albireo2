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

#include "kadas/core/kadas_core.h"

class QgsPointXY;
class QgsCoordinateReferenceSystem;

class KADAS_CORE_EXPORT KadasCoordinateFormat : public QObject
{
    Q_OBJECT
  public:
    enum class Format SIP_MONKEYPATCH_SCOPEENUM : int
    {
      Default,
      DegMinSec,
      DegMin,
      DecDeg,
      UTM,
      MGRS
    };
    Q_ENUM( Format )

    static KadasCoordinateFormat *instance();
    KadasCoordinateFormat::Format getCoordinateDisplayFormat() const;
    const QString &getCoordinateDisplayCrs() const;
    Qgis::DistanceUnit getHeightDisplayUnit() const { return mHeightUnit; }

    QString getDisplayString( const QgsPointXY &p, const QgsCoordinateReferenceSystem &sSrs ) const;
    static QString getDisplayString( const QgsPointXY &p, const QgsCoordinateReferenceSystem &sSrs, KadasCoordinateFormat::Format format, const QString &epsg );

    double getHeightAtPos( const QgsPointXY &p, const QgsCoordinateReferenceSystem &crs, QString *errMsg = 0 );
#ifndef SIP_RUN
    [[deprecated( "Use KadasCoordinateUtils::getHeightAtPos." )]]
#endif
    static double getHeightAtPos( const QgsPointXY &p, const QgsCoordinateReferenceSystem &crs, Qgis::DistanceUnit unit, QString *errMsg = 0 );

    QgsPointXY parseCoordinate( const QString &text, KadasCoordinateFormat::Format format, bool &valid ) const;

  public slots:
    void setCoordinateDisplayFormat( KadasCoordinateFormat::Format format, const QString &epsg );
    void setHeightDisplayUnit( Qgis::DistanceUnit heightUnit );

  signals:
    void coordinateDisplayFormatChanged( KadasCoordinateFormat::Format format, const QString &epsg );
    void heightDisplayUnitChanged( Qgis::DistanceUnit heightUnit );

  private:
    KadasCoordinateFormat() SIP_FORCE;

    Format mFormat;
    QString mEpsg;
    Qgis::DistanceUnit mHeightUnit;
};

#endif // KADASCOORDINATEFORMAT_H
