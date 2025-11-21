/***************************************************************************
    kadastextitem.h
    ---------------
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

#ifndef KADASTEXTITEM_H
#define KADASTEXTITEM_H

#include "kadas/gui/mapitems/kadaspointitem.h"

class QgsAnnotationPointTextItem;

class KADAS_GUI_EXPORT KadasTextItem : public KadasPointItem
{
    Q_OBJECT
    Q_PROPERTY( QString text READ text WRITE setText )
    Q_PROPERTY( QColor color READ color WRITE setColor )
    Q_PROPERTY( QFont font READ font WRITE setFont )

  public:
    KadasTextItem( const QgsCoordinateReferenceSystem &crs );

    QString itemName() const override { return tr( "Text" ); }

    void setText( const QString &text );
    const QString &text() const { return mText; }

    void setPoint( const QgsPointXY &point ) override;
    QgsPointXY point() const override;

    void setColor( const QColor &color );
    QColor color() const { return mColor; }

    void setFont( const QFont &font );
    const QFont &font() const { return mFont; }

    virtual QgsRectangle boundingBox() const override;
    virtual void render( QgsRenderContext &context ) const override;


    // virtual EditContext getEditContext(const KadasMapPos &pos, const QgsMapSettings &mapSettings) const override;
    // virtual void edit(const EditContext &context, const KadasMapPos &newPoint, const QgsMapSettings &mapSettings) override;
    // virtual void edit(const EditContext &context, const AttribValues &values, const QgsMapSettings &mapSettings) override;
    // virtual AttribValues editAttribsFromPosition(const EditContext &context, const KadasMapPos &pos, const QgsMapSettings &mapSettings) const override;
    // virtual KadasMapPos positionFromEditAttribs(const EditContext &context, const AttribValues &values, const QgsMapSettings &mapSettings) const override;

#ifndef SIP_RUN
    QString asKml( const QgsRenderContext &context, QuaZip *kmzZip = nullptr ) const override;
#endif

  protected:
    virtual KadasMapItem *_clone() const override;
    virtual void writeXmlPrivate( QDomElement &element ) const override;
    virtual void readXmlPrivate( const QDomElement &element ) override;

  private:
    void updateQgsAnnotation() override;

    QgsAnnotationPointTextItem *mQgsItem = nullptr;

    QString mText;
    QColor mColor;
    QFont mFont;
};

#endif // KADASTEXTITEM_H
