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

#include "kadas/gui/mapitems/kadasrectangleitembase.h"


class KADAS_GUI_EXPORT KadasTextItem : public KadasRectangleItemBase
{
    Q_OBJECT
    Q_PROPERTY( QString text READ text WRITE setText )
    Q_PROPERTY( QColor outlineColor READ outlineColor WRITE setOutlineColor )
    Q_PROPERTY( QColor fillColor READ fillColor WRITE setFillColor )
    Q_PROPERTY( QFont font READ font WRITE setFont )
    Q_PROPERTY( bool frameAutoResize READ frameAutoResize WRITE setFrameAutoResize )

  public:
    KadasTextItem( const QgsCoordinateReferenceSystem &crs );

    QString itemName() const override { return tr( "Text" ); }

    void setText( const QString &text );
    const QString &text() const { return mText; }
    void setFillColor( const QColor &c );
    QColor fillColor() const { return mFillColor; }
    void setOutlineColor( const QColor &c );
    QColor outlineColor() const { return mOutlineColor; }
    void setFont( const QFont &font );
    const QFont &font() const { return mFont; }
    void setFrameAutoResize( bool frameAutoResize );
    bool frameAutoResize() const { return mFrameAutoResize; }
    void setAngle( double angle );

    QImage symbolImage() const override;

#ifndef SIP_RUN
    QString asKml( const QgsRenderContext &context, QuaZip *kmzZip = nullptr ) const override;
#endif

  private:
    QString mText;
    QColor mOutlineColor;
    QColor mFillColor;
    QFont mFont;
    bool mFrameAutoResize = true;

    KadasMapItem *_clone() const override SIP_FACTORY { return new KadasTextItem( crs() ); }

  protected:
    virtual void renderPrivate( QgsRenderContext &context, const QPointF &center, const QRect &rect, double dpiScale ) const override;
    virtual void editPrivate( const KadasMapPos &newPoint, const QgsMapSettings &mapSettings ) override;
    void populateContextMenuPrivate( QMenu *menu, const EditContext &context, const KadasMapPos &clickPos, const QgsMapSettings &mapSettings ) override;
};

#endif // KADASTEXTITEM_H
