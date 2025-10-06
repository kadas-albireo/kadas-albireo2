/***************************************************************************
    kadasanchoreditem.h
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

#ifndef KADASANCHOREDITEM_H
#define KADASANCHOREDITEM_H

#include "kadas/gui/mapitems/kadasmapitem.h"

class KADAS_GUI_EXPORT KadasAnchoredItem : public KadasMapItem SIP_ABSTRACT {
  Q_OBJECT
  Q_PROPERTY(double anchorX READ anchorX WRITE setAnchorX)
  Q_PROPERTY(double anchorY READ anchorY WRITE setAnchorY)

public:
  KadasAnchoredItem(const QgsCoordinateReferenceSystem &crs);

  // Item anchor point, as factors of its width/height
  double anchorX() const { return mAnchorX; }
  void setAnchorX(double anchorX);
  double anchorY() const { return mAnchorY; }
  void setAnchorY(double anchorY);

  KadasItemRect boundingBox() const override;
  Margin margin() const override;
  QList<KadasMapItem::Node>
  nodes(const QgsMapSettings &settings) const override;
  bool intersects(const KadasMapRect &rect, const QgsMapSettings &settings,
                  bool contains = false) const override;

  bool startPart(const KadasMapPos &firstPoint,
                 const QgsMapSettings &mapSettings) override;
  bool startPart(const AttribValues &values,
                 const QgsMapSettings &mapSettings) override;
  void setCurrentPoint(const KadasMapPos &p,
                       const QgsMapSettings &mapSettings) override;
  void setCurrentAttributes(const AttribValues &values,
                            const QgsMapSettings &mapSettings) override;
  bool continuePart(const QgsMapSettings &mapSettings) override;
  void endPart() override;

  AttribDefs drawAttribs() const override;
  AttribValues
  drawAttribsFromPosition(const KadasMapPos &pos,
                          const QgsMapSettings &mapSettings) const override;
  KadasMapPos
  positionFromDrawAttribs(const AttribValues &values,
                          const QgsMapSettings &mapSettings) const override;

  EditContext getEditContext(const KadasMapPos &pos,
                             const QgsMapSettings &mapSettings) const override;
  void edit(const EditContext &context, const KadasMapPos &newPoint,
            const QgsMapSettings &mapSettings) override;
  void edit(const EditContext &context, const AttribValues &values,
            const QgsMapSettings &mapSettings) override;

  AttribValues
  editAttribsFromPosition(const EditContext &context, const KadasMapPos &pos,
                          const QgsMapSettings &mapSettings) const override;
  KadasMapPos
  positionFromEditAttribs(const EditContext &context,
                          const AttribValues &values,
                          const QgsMapSettings &mapSettings) const override;

  KadasItemPos position() const override { return constState()->pos; }
  void setPosition(const KadasItemPos &pos) override;

  void setAngle(double angle);

  struct KADAS_GUI_EXPORT State : KadasMapItem::State {
    KadasItemPos pos;
    double angle;
    QSize size;
    void assign(const KadasMapItem::State *other) override {
      *this = *static_cast<const State *>(other);
    }
    State *clone() const override SIP_FACTORY { return new State(*this); }
    QJsonObject serialize() const override;
    bool deserialize(const QJsonObject &json) override;
  };
  const State *constState() const { return static_cast<State *>(mState); }

protected:
  enum AttribIds SIP_SKIP { AttrX, AttrY, AttrA };
  double mAnchorX = 0.5;
  double mAnchorY = 0.5;

  State *state() { return static_cast<State *>(mState); }
  State *createEmptyState() const override SIP_FACTORY { return new State(); }
  QList<KadasMapPos> rotatedCornerPoints(double angle,
                                         const QgsMapSettings &settings) const;

  static void rotateNodeRenderer(QPainter *painter, const QPointF &screenPoint,
                                 int nodeSize) SIP_SKIP;
};

#endif // KADASANCHOREDITEM_H
