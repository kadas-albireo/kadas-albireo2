/***************************************************************************
    kadasredliningitemeditor.cpp
    ----------------------------
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

#include <QGridLayout>

#include <qgis/qgssettings.h>
#include <qgis/qgssymbollayerutils.h>

#include <kadas/core/mapitems/kadasgeometryitem.h>

#include <kadas/gui/mapitemeditors/kadasredliningitemeditor.h>


KadasRedliningItemEditor::KadasRedliningItemEditor(KadasMapItem* item)
  : KadasMapItemEditor(item)
{
  mUi.setupUi(this);

  KadasGeometryItem* geometryItem = dynamic_cast<KadasGeometryItem*>(mItem);
  if(geometryItem) {
    mUi.mLabelSize->setText(geometryItem->geometryType() == QgsWkbTypes::PointGeometry ? tr("Size:") : tr("Border width:"));
  }
  mUi.mSpinBoxSize->setRange( 1, 100 );
  mUi.mSpinBoxSize->setValue( QgsSettings().value( "/Redlining/size", 1 ).toInt() );
  connect( mUi.mSpinBoxSize, qOverload<int>(&QSpinBox::valueChanged), this, &KadasRedliningItemEditor::saveOutlineWidth );

  mUi.mToolButtonBorderColor->setAllowOpacity( true );
  mUi.mToolButtonBorderColor->setShowNoColor( true );
  mUi.mToolButtonBorderColor->setProperty( "settings_key", "outline_color" );
  QColor initialOutlineColor = QgsSymbolLayerUtils::decodeColor( QgsSettings().value( "/Redlining/outline_color", "0,0,0,255" ).toString() );
  mUi.mToolButtonBorderColor->setColor( initialOutlineColor );
  connect( mUi.mToolButtonBorderColor, &QgsColorButton::colorChanged, this, &KadasRedliningItemEditor::saveColor );

  mUi.mComboBoxOutlineStyle->setProperty( "settings_key", "outline_style" );
  mUi.mComboBoxOutlineStyle->addItem( createOutlineStyleIcon( Qt::NoPen ), QString(), static_cast<int>( Qt::NoPen ) );
  mUi.mComboBoxOutlineStyle->addItem( createOutlineStyleIcon( Qt::SolidLine ), QString(), static_cast<int>( Qt::SolidLine ) );
  mUi.mComboBoxOutlineStyle->addItem( createOutlineStyleIcon( Qt::DashLine ), QString(), static_cast<int>( Qt::DashLine ) );
  mUi.mComboBoxOutlineStyle->addItem( createOutlineStyleIcon( Qt::DashDotLine ), QString(), static_cast<int>( Qt::DashDotLine ) );
  mUi.mComboBoxOutlineStyle->addItem( createOutlineStyleIcon( Qt::DotLine ), QString(), static_cast<int>( Qt::DotLine ) );
  mUi.mComboBoxOutlineStyle->setCurrentIndex( QgsSettings().value( "/Redlining/outline_style", "1" ).toInt() );
  connect( mUi.mComboBoxOutlineStyle, qOverload<int>(&QComboBox::currentIndexChanged), this, &KadasRedliningItemEditor::saveStyle );

  mUi.mToolButtonFillColor->setAllowOpacity( true );
  mUi.mToolButtonFillColor->setShowNoColor( true );
  mUi.mToolButtonFillColor->setProperty( "settings_key", "fill_color" );
  QColor initialFillColor = QgsSymbolLayerUtils::decodeColor( QgsSettings().value( "/Redlining/fill_color", "255,0,0,255" ).toString() );
  mUi.mToolButtonFillColor->setColor( initialFillColor );
  connect( mUi.mToolButtonFillColor, &QgsColorButton::colorChanged, this, &KadasRedliningItemEditor::saveColor );

  mUi.mComboBoxFillStyle->setProperty( "settings_key", "fill_style" );
  mUi.mComboBoxFillStyle->addItem( createFillStyleIcon( Qt::NoBrush ), QString(), static_cast<int>( Qt::NoBrush ) );
  mUi.mComboBoxFillStyle->addItem( createFillStyleIcon( Qt::SolidPattern ), QString(), static_cast<int>( Qt::SolidPattern ) );
  mUi.mComboBoxFillStyle->addItem( createFillStyleIcon( Qt::HorPattern ), QString(), static_cast<int>( Qt::HorPattern ) );
  mUi.mComboBoxFillStyle->addItem( createFillStyleIcon( Qt::VerPattern ), QString(), static_cast<int>( Qt::VerPattern ) );
  mUi.mComboBoxFillStyle->addItem( createFillStyleIcon( Qt::BDiagPattern ), QString(), static_cast<int>( Qt::BDiagPattern ) );
  mUi.mComboBoxFillStyle->addItem( createFillStyleIcon( Qt::DiagCrossPattern ), QString(), static_cast<int>( Qt::DiagCrossPattern ) );
  mUi.mComboBoxFillStyle->addItem( createFillStyleIcon( Qt::FDiagPattern ), QString(), static_cast<int>( Qt::FDiagPattern ) );
  mUi.mComboBoxFillStyle->addItem( createFillStyleIcon( Qt::CrossPattern ), QString(), static_cast<int>( Qt::CrossPattern ) );
  mUi.mComboBoxFillStyle->setCurrentIndex( QgsSettings().value( "/Redlining/fill_style", "1" ).toInt() );
  connect( mUi.mComboBoxFillStyle, qOverload<int>(&QComboBox::currentIndexChanged), this, &KadasRedliningItemEditor::saveStyle );

  connect( this, &KadasRedliningItemEditor::styleChanged, this, [this]{ syncWidgetToItem(); });

  toggleItemMeasurements(true);
  mItem->setSelected(true);
}

void KadasRedliningItemEditor::syncItemToWidget()
{
  KadasGeometryItem* geometryItem = dynamic_cast<KadasGeometryItem*>(mItem);
  if(!geometryItem) {
    return;
  }

  mUi.mSpinBoxSize->blockSignals(true);
  mUi.mSpinBoxSize->setValue(geometryItem->outlineWidth());
  mUi.mSpinBoxSize->blockSignals(false);

  mUi.mToolButtonBorderColor->blockSignals(true);
  mUi.mToolButtonBorderColor->setColor(geometryItem->outlineColor());
  mUi.mToolButtonBorderColor->blockSignals(false);

  mUi.mToolButtonFillColor->blockSignals(true);
  mUi.mToolButtonFillColor->setColor(geometryItem->fillColor());
  mUi.mToolButtonFillColor->blockSignals(false);

  mUi.mComboBoxOutlineStyle->blockSignals(true);
  mUi.mComboBoxOutlineStyle->setCurrentIndex(mUi.mComboBoxOutlineStyle->findData(static_cast<int>(geometryItem->lineStyle())));
  mUi.mComboBoxOutlineStyle->blockSignals(false);

  mUi.mComboBoxFillStyle->blockSignals(true);
  mUi.mComboBoxFillStyle->setCurrentIndex(mUi.mComboBoxFillStyle->findData(static_cast<int>(geometryItem->brushStyle())));
  mUi.mComboBoxFillStyle->blockSignals(false);
}

void KadasRedliningItemEditor::syncWidgetToItem()
{
  KadasGeometryItem* geometryItem = dynamic_cast<KadasGeometryItem*>(mItem);
  if(!geometryItem) {
    return;
  }

  int outlineWidth = mUi.mSpinBoxSize->value();
  QColor outlineColor = mUi.mToolButtonBorderColor->color();
  QColor fillColor = mUi.mToolButtonFillColor->color();
  Qt::PenStyle lineStyle = static_cast<Qt::PenStyle>( mUi.mComboBoxOutlineStyle->itemData( mUi.mComboBoxOutlineStyle->currentIndex() ).toInt() );
  Qt::BrushStyle brushStyle = static_cast<Qt::BrushStyle>( mUi.mComboBoxFillStyle->itemData( mUi.mComboBoxFillStyle->currentIndex() ).toInt() );

  geometryItem->setOutlineWidth(outlineWidth);
  geometryItem->setOutlineColor(outlineColor);
  geometryItem->setFillColor(fillColor);
  geometryItem->setLineStyle(lineStyle);
  geometryItem->setBrushStyle(brushStyle);

  geometryItem->setIconOutlineWidth(outlineWidth / 4);
  geometryItem->setIconSize(10 + 2 * outlineWidth);
  geometryItem->setIconOutlineColor(outlineColor);
  geometryItem->setIconFillColor(fillColor);
  geometryItem->setIconLineStyle(lineStyle);
  geometryItem->setIconBrushStyle(brushStyle);
}

KadasRedliningItemEditor::~KadasRedliningItemEditor()
{
  toggleItemMeasurements(false);
  if(mItem) {
    mItem->setSelected(false);
  }
}

void KadasRedliningItemEditor::toggleItemMeasurements(bool enabled)
{
  KadasGeometryItem* geometryItem = qobject_cast<KadasGeometryItem*>(mItem);
  if(!geometryItem) {
    return;
  }
  geometryItem->setMeasurementsEnabled(enabled);
}

void KadasRedliningItemEditor::saveColor()
{
  QgsColorButton* btn = qobject_cast<QgsColorButton*>( QObject::sender() );
  QString key = QString( "/Redlining/" ) + btn->property( "settings_key" ).toString();
  QgsSettings().setValue( key, QgsSymbolLayerUtils::encodeColor( btn->color() ) );
  emit styleChanged();
}

void KadasRedliningItemEditor::saveOutlineWidth()
{
  QgsSettings().setValue( "/Redlining/size", mUi.mSpinBoxSize->value() );
  emit styleChanged();
}

void KadasRedliningItemEditor::saveStyle()
{
  QComboBox* combo = qobject_cast<QComboBox*>( QObject::sender() );
  QString key = QString( "/Redlining/" ) + combo->property( "settings_key" ).toString();
  QgsSettings().setValue( key, combo->currentIndex() );
  emit styleChanged();
}

QIcon KadasRedliningItemEditor::createOutlineStyleIcon( Qt::PenStyle style )
{
  QPixmap pixmap( 16, 16 );
  pixmap.fill( Qt::transparent );
  QPainter painter( &pixmap );
  painter.setRenderHint( QPainter::Antialiasing );
  QPen pen;
  pen.setStyle( style );
  pen.setColor( Qt::black );
  pen.setWidth( 2 );
  painter.setPen( pen );
  painter.drawLine( 0, 8, 16, 8 );
  return pixmap;
}

QIcon KadasRedliningItemEditor::createFillStyleIcon( Qt::BrushStyle style )
{
  QPixmap pixmap( 16, 16 );
  pixmap.fill( Qt::transparent );
  QPainter painter( &pixmap );
  painter.setRenderHint( QPainter::Antialiasing );
  QBrush brush;
  brush.setStyle( style );
  brush.setColor( Qt::black );
  painter.fillRect( 0, 0, 16, 16, brush );
  return pixmap;
}
