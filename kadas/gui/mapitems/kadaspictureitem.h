/***************************************************************************
    kadaspictureitem.h
    ------------------
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

#ifndef KADASPICTUREITEM_H
#define KADASPICTUREITEM_H

#include <kadas/gui/mapitems/kadasmapitem.h>


class KADAS_GUI_EXPORT KadasPictureItem : public KadasMapItem
{
  public:
    KadasPictureItem( const QgsCoordinateReferenceSystem &crs, QObject *parent = nullptr );
    void setFilePath( const QString &path, const QgsPointXY &fallbackPos, bool ignoreExiv = false, double offsetX = 0, double offsetY = 50 );

    QgsRectangle boundingBox() const override;
    QRect margin() const override;
    QList<KadasMapItem::Node> nodes( const QgsMapSettings &settings ) const override;
    bool intersects( const QgsRectangle &rect, const QgsMapSettings &settings ) const override;
    void render( QgsRenderContext &context ) const override;

    bool startPart( const QgsPointXY &firstPoint, const QgsMapSettings &mapSettings ) override;
    bool startPart( const AttribValues &values, const QgsMapSettings &mapSettings ) override;
    void setCurrentPoint( const QgsPointXY &p, const QgsMapSettings &mapSettings ) override;
    void setCurrentAttributes( const AttribValues &values, const QgsMapSettings &mapSettings ) override;
    bool continuePart( const QgsMapSettings &mapSettings ) override;
    void endPart() override;

    AttribDefs drawAttribs() const override;
    AttribValues drawAttribsFromPosition( const QgsPointXY &pos ) const override;
    QgsPointXY positionFromDrawAttribs( const AttribValues &values ) const override;

    EditContext getEditContext( const QgsPointXY &pos, const QgsMapSettings &mapSettings ) const override;
    void edit( const EditContext &context, const QgsPointXY &newPoint, const QgsMapSettings &mapSettings ) override;
    void edit( const EditContext &context, const AttribValues &values, const QgsMapSettings &mapSettings ) override;

    AttribValues editAttribsFromPosition( const EditContext &context, const QgsPointXY &pos ) const override;
    QgsPointXY positionFromEditAttribs( const EditContext &context, const AttribValues &values, const QgsMapSettings &mapSettings ) const override;

    struct State : KadasMapItem::State
    {
      QgsPointXY pos;
      double angle;
      QSize size;
      void assign( const KadasMapItem::State *other ) override { *this = *static_cast<const State *>( other ); }
      State *clone() const override SIP_FACTORY { return new State( *this ); }
    };
    const State *constState() const { return static_cast<State *>( mState ); }

  protected:
    State *createEmptyState() const override { return new State(); }

  private:
    enum AttribIds {AttrX, AttrY};
    QString mFilePath;
    double mOffsetX = 0;
    double mOffsetY = 0;
    QImage mImage;

    static constexpr int sFramePadding = 4;
    static constexpr int sArrowWidth = 6;

    State *state() { return static_cast<State *>( mState ); }

    QList<QgsPointXY> cornerPoints( const QgsPointXY &anchor, double mup = 1. ) const;
    static bool readGeoPos( const QString &filePath, QgsPointXY &wgsPos );
};

#endif // KADASPICTUREITEM_H
