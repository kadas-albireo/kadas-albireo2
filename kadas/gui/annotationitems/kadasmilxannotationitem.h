/***************************************************************************
    kadasmilxannotationitem.h
    -------------------------
    copyright            : (C) 2026 by Denis Rouzaud
    email                : denis at opengis dot ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef KADASMILXANNOTATIONITEM_H
#define KADASMILXANNOTATIONITEM_H

#include <QMap>
#include <QPoint>
#include <QString>

#include <qgis/qgsannotationitem.h>
#include <qgis/qgspointxy.h>
#include <qgis/qgsrectangle.h>

#include "kadas/gui/kadas_gui.h"
#include "kadas/gui/milx/kadasmilxclient.h"

class QgsMapSettings;
class QgsCoordinateTransform;
class QgsCoordinateTransformContext;
class QgsAnnotationLayer;

/**
 * \ingroup gui
 * \brief Kadas MilX (military symbology) annotation item.
 *
 * Holds an MSS string and a list of WGS84 points; rendering, hit-testing and
 * editing delegate to \c KadasMilxClient. Type id: \c "kadas:milx".
 */
class KADAS_GUI_EXPORT KadasMilxAnnotationItem : public QgsAnnotationItem
{
  public:
    KadasMilxAnnotationItem();

    static QString itemTypeId() { return QStringLiteral( "kadas:milx" ); }

    QString type() const override;
    QgsRectangle boundingBox() const override;
    Qgis::AnnotationItemFlags flags() const override;
    void render( QgsRenderContext &context, QgsFeedback *feedback ) override;
    bool writeXml( QDomElement &element, QDomDocument &document, const QgsReadWriteContext &context ) const override;
    bool readXml( const QDomElement &element, const QgsReadWriteContext &context ) override;
    KadasMilxAnnotationItem *clone() const override;

    static KadasMilxAnnotationItem *create();

    /**
     * Write this item as a `<MilXGraphic>` DOM element (legacy MilXly schema).
     * \a symbolSize is the layer's symbol size in px (for the user-offset factor).
     */
    void writeMilx( QDomDocument &doc, QDomElement &itemElement, int symbolSize ) const;

    /**
     * Construct a \c KadasMilxAnnotationItem from a `<MilXGraphic>` element.
     * \a crst projects source coordinates to EPSG:4326; caller takes ownership.
     */
    static KadasMilxAnnotationItem *fromMilx( const QDomElement &itemElement, const QgsCoordinateTransform &crst, int symbolSize );

    /**
     * Serialize all `kadas:milx` items in \a annoLayer into \a milxLayerEl as a
     * `<MilXLayer>` block. Returns the number of items emitted.
     */
    static int exportLayerToMilxly( QgsAnnotationLayer *annoLayer, QDomElement &milxLayerEl, int dpi );

    /**
     * Populate \a annoLayer with `kadas:milx` items parsed from \a milxLayerEl.
     * Returns true on success; sets \a errorMsg on failure.
     */
    static bool importLayerFromMilxly( QgsAnnotationLayer *annoLayer, const QDomElement &milxLayerEl, int dpi, const QgsCoordinateTransformContext &transformContext, QString &errorMsg );

    QString mssString() const { return mMssString; }
    void setMssString( const QString &mssString ) { mMssString = mssString; }

    QString militaryName() const { return mMilitaryName; }
    void setMilitaryName( const QString &militaryName ) { mMilitaryName = militaryName; }

    QString symbolType() const { return mSymbolType; }
    void setSymbolType( const QString &symbolType ) { mSymbolType = symbolType; }

    int minNumPoints() const { return mMinNumPoints; }
    void setMinNumPoints( int minNumPoints ) { mMinNumPoints = minNumPoints; }

    bool hasVariablePoints() const { return mHasVariablePoints; }
    void setHasVariablePoints( bool hasVariablePoints ) { mHasVariablePoints = hasVariablePoints; }

    //! Points, stored in EPSG:4326 (MilX is always WGS84).
    QList<QgsPointXY> points() const { return mPoints; }
    void setPoints( const QList<QgsPointXY> &points ) { mPoints = points; }

    //! Indices into points() that libmss reports as draggable control points.
    QList<int> controlPoints() const { return mControlPoints; }
    void setControlPoints( const QList<int> &cp ) { mControlPoints = cp; }

    //! Symbol attribute values (width/length/radius/attitude) in WGS84 m/deg, keyed by \c KadasMilxAttrType.
#ifndef SIP_RUN
    QMap<KadasMilxAttrType, double> attributes() const { return mAttributes; }
    void setAttributes( const QMap<KadasMilxAttrType, double> &a ) { mAttributes = a; }

    //! Per-attribute control points reported by libmss, in EPSG:4326.
    QMap<KadasMilxAttrType, QgsPointXY> attributePoints() const { return mAttributePoints; }
    void setAttributePoints( const QMap<KadasMilxAttrType, QgsPointXY> &p ) { mAttributePoints = p; }
#endif

    //! User-applied screen-space offset (drag of the symbol graphic).
    QPoint userOffset() const { return mUserOffset; }
    void setUserOffset( const QPoint &offset ) { mUserOffset = offset; }

    //! Number of physical clicks made during interactive draw.
    int pressedPoints() const { return mPressedPoints; }
    void setPressedPoints( int n ) { mPressedPoints = n; }

    enum class DrawStatus
    {
      Empty,
      Drawing,
      Finished
    };
    DrawStatus drawStatus() const { return mDrawStatus; }
    void setDrawStatus( DrawStatus s ) { mDrawStatus = s; }

    // ---- libmss helpers ---------------------------------------------------

    //! Builds the libmss \c NPointSymbol for this item in the map's screen coordinates.
#ifndef SIP_RUN
    KadasMilxClient::NPointSymbol toSymbol( const QgsMapSettings &mapSettings, bool colored = true ) const;

    //! Replaces this item's points / attributes from a libmss result graphic.
    void applySymbolResult( const QgsMapSettings &mapSettings, const KadasMilxClient::NPointSymbolGraphic &result );
#endif

    //! On-screen extent of \a mapSettings' visible map extent (in device pixels).
    static QRect computeScreenExtent( const QgsMapSettings &mapSettings );

    //! True when the symbol has more than one geometry point or carries a shape attribute.
    bool isMultiPoint() const { return mPoints.size() > 1 || !mAttributes.isEmpty(); }

  private:
    QString mMssString;
    QString mMilitaryName;
    QString mSymbolType;
    int mMinNumPoints = 1;
    bool mHasVariablePoints = false;
    QList<QgsPointXY> mPoints;
    QList<int> mControlPoints;
    QMap<KadasMilxAttrType, double> mAttributes;
    QMap<KadasMilxAttrType, QgsPointXY> mAttributePoints;
    QPoint mUserOffset;
    int mPressedPoints = 0;
    DrawStatus mDrawStatus = DrawStatus::Finished;

    //! Pixel-per-metre scale at the first point (converts libmss attribute values to/from screen pixels).
    double metersToPixels( const QgsMapSettings &mapSettings ) const;
};

#endif // KADASMILXANNOTATIONITEM_H
