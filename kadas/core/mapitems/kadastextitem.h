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

#include <kadas/core/kadas_core.h>
#include <kadas/core/mapitems/kadasmapitem.h>


class KADAS_CORE_EXPORT KadasTextItem : public KadasMapItem
{
public:
  KadasTextItem(const QgsCoordinateReferenceSystem& crs, QObject* parent = nullptr);

  void setText(const QString& text);
  const QString& text() const{ return mText; }
  void setFillColor( const QColor& c );
  QColor fillColor() const{ return mFillColor; }
  void setOutlineColor( const QColor& c );
  QColor outlineColor() const{ return mOutlineColor; }
  void setFont( const QFont& font );
  const QFont& font() const{ return mFont; }
  void setRotation(double angle);
  double rotation() const{ return mAngle; }

  QgsRectangle boundingBox() const override;
  QRect margin() const override;
  QList<Node> nodes(const QgsMapSettings& settings) const override;
  bool intersects( const QgsRectangle& rect, const QgsMapSettings& settings ) const override;
  void render( QgsRenderContext &context ) const override;

  bool startPart(const QgsPointXY& firstPoint) override;
  bool startPart(const AttribValues& values) override;
  void setCurrentPoint(const QgsPointXY& p, const QgsMapSettings* mapSettings=nullptr) override;
  void setCurrentAttributes(const AttribValues& values) override;
  bool continuePart() override;
  void endPart() override;

  AttribDefs drawAttribs() const override;
  AttribValues drawAttribsFromPosition(const QgsPointXY& pos) const override;
  QgsPointXY positionFromDrawAttribs(const AttribValues& values) const override;

  EditContext getEditContext(const QgsPointXY& pos, const QgsMapSettings& mapSettings) const override;
  void edit(const EditContext& context, const QgsPointXY& newPoint, const QgsMapSettings* mapSettings=nullptr) override;
  void edit(const EditContext& context, const AttribValues& values) override;

  AttribValues editAttribsFromPosition(const EditContext& context, const QgsPointXY& pos) const override;
  QgsPointXY positionFromEditAttribs(const EditContext& context, const AttribValues& values, const QgsMapSettings& mapSettings) const override;

  struct State : KadasMapItem::State {
    QgsPointXY pos;
    void assign(const KadasMapItem::State* other) override { *this = *static_cast<const State*>(other); }
    State* clone() const override { return new State(*this); }
  };
  const State* state() const{ return static_cast<State*>(mState); }

private:
  enum AttribIds {AttrX, AttrY};
  QString mText;
  QColor mOutlineColor;
  QColor mFillColor;
  QFont mFont;
  double mAngle = 0.0;

  State* state(){ return static_cast<State*>(mState); }
  State* createEmptyState() const override { return new State(); }
  void recomputeDerived() override;

  QList<QgsPointXY> rotatedCornerPoints(double mup = 1.) const;
};

#endif // KADASTEXTITEM_H
