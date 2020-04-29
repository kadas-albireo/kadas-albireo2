/***************************************************************************
    kadasgeometryitem.h
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

#ifndef KADASGEOMETRYITEM_H
#define KADASGEOMETRYITEM_H

#include <QBrush>
#include <QPen>

#include <qgis/qgsabstractgeometry.h>
#include <qgis/qgsdistancearea.h>

#include <kadas/gui/mapitems/kadasmapitem.h>

struct QgsVertexId;

class KADAS_GUI_EXPORT KadasGeometryItem : public KadasMapItem SIP_ABSTRACT
{
    Q_OBJECT
    Q_PROPERTY( QPen outline READ outline WRITE setOutline )
    Q_PROPERTY( QBrush fill READ fill WRITE setFill )
    Q_PROPERTY( int iconSize READ iconSize WRITE setIconSize )
    Q_PROPERTY( IconType iconType READ iconType WRITE setIconType )
    Q_PROPERTY( QPen iconOutline READ iconOutline WRITE setIconOutline )
    Q_PROPERTY( QBrush iconFill READ iconFill WRITE setIconFill )

  public:
    enum IconType
    {
      /**
      * No icon is used
      */
      ICON_NONE,
      /**
       * A cross is used to highlight points (+)
       */
      ICON_CROSS,
      /**
       * A cross is used to highlight points (x)
       */
      ICON_X,
      /**
       * A box is used to highlight points (□)
       */
      ICON_BOX,
      /**
       * A circle is used to highlight points (○)
       */
      ICON_CIRCLE,
      /**
       * A full box is used to highlight points (■)
       */
      ICON_FULL_BOX,
      /**
       * A triangle is used to highlight points (△)
       */
      ICON_TRIANGLE,
      /**
       * A full triangle is used to highlight points (▲)
       */
      ICON_FULL_TRIANGLE
    };

    KadasGeometryItem( const QgsCoordinateReferenceSystem &crs );
    ~KadasGeometryItem();

    KadasItemRect boundingBox() const override;
    Margin margin() const override;
    QList<KadasMapItem::Node> nodes( const QgsMapSettings &settings ) const override;
    bool intersects( const KadasMapRect &rect, const QgsMapSettings &settings ) const override;
    QPair<KadasMapPos, double> closestPoint( const KadasMapPos &pos, const QgsMapSettings &settings ) const override;
    void render( QgsRenderContext &context ) const override;
#ifndef SIP_RUN
    QString asKml( const QgsRenderContext &context, QuaZip *kmzZip = nullptr ) const override;
#endif

    void clear() override;
    void setState( const State *state ) override;

    virtual QgsWkbTypes::GeometryType geometryType() const = 0;
    // Geometry in item CRS
    virtual void addPartFromGeometry( const QgsAbstractGeometry &geom ) = 0;

    QPen outline() const { return mPen; }
    void setOutline( const QPen &pen );
    QBrush fill() const { return mBrush; }
    void setFill( const QBrush &brush );
    int iconSize() const { return mIconSize; }
    void setIconSize( int iconSize );
    IconType iconType() const { return mIconType; }
    void setIconType( IconType iconType );
    QPen iconOutline() const { return mIconPen; }
    void setIconOutline( const QPen &iconPen );
    QBrush iconFill() const { return mIconBrush; }
    void setIconFill( const QBrush &iconBrush );

    void setMeasurementsEnabled( bool enabled, QgsUnitTypes::DistanceUnit baseUnit = QgsUnitTypes::DistanceMeters );
    QString getTotalMeasurement() const { return mTotalMeasurement; }

    // Geometry in item CRS
    const QgsAbstractGeometry *geometry() const { return mGeometry; }

  signals:
    void geometryChanged();

  protected:
    QgsAbstractGeometry *mGeometry = nullptr;

    QPen mPen;
    QBrush mBrush;
    int mIconSize;
    IconType mIconType;
    QPen mIconPen;
    QBrush mIconBrush;

    QgsDistanceArea mDa;
    bool mMeasureGeometry = false;
    QgsUnitTypes::DistanceUnit mBaseUnit = QgsUnitTypes::DistanceMeters;

    QString mTotalMeasurement;


    void setInternalGeometry( QgsAbstractGeometry *geom );

    void drawVertex( QPainter *p, double x, double y ) const;
    QgsUnitTypes::DistanceUnit distanceBaseUnit() const;
    QgsUnitTypes::AreaUnit areaBaseUnit() const;
    QString formatLength( double value, QgsUnitTypes::DistanceUnit unit ) const;
    QString formatArea( double value, QgsUnitTypes::AreaUnit unit ) const;
    QString formatAngle( double value, QgsUnitTypes::AngleUnit unit ) const;
    void addMeasurements( const QStringList &measurements, const KadasItemPos &mapPos, bool center = true );

    QgsVertexId insertionPoint( const QList<QList<KadasItemPos>> &points, const KadasItemPos &testPos ) const;

    virtual void recomputeDerived() = 0;
    virtual void measureGeometry() {}

  private slots:
    void updateMeasurements();

  private:

    struct MeasurementLabel
    {
      QString string;
      KadasItemPos pos;
      int width;
      int height;
      bool center;
    };
    QList<MeasurementLabel> mMeasurementLabels;

    static void registerMetaTypes();
};

Q_DECLARE_METATYPE( KadasGeometryItem::IconType )

#endif // KADASGEOMETRYITEM_H
