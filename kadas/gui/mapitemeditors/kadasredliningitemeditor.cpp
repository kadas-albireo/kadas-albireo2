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

#include <qgis/qgssettings.h>
#include <qgis/qgssymbollayerutils.h>

#include "kadas/gui/mapitems/kadasgeometryitem.h"
#include "kadas/gui/mapitems/kadaspointitem.h"
#include "kadas/gui/mapitemeditors/kadasredliningitemeditor.h"


KadasRedliningItemEditor::KadasRedliningItemEditor( KadasMapItem *item )
  : KadasMapItemEditor( item )
{
  mUi.setupUi( this );

  KadasGeometryItem *geometryItem = dynamic_cast<KadasGeometryItem *>( mItem );
  if ( geometryItem )
  {
    mUi.mLabelSize->setText( geometryItem->geometryType() == Qgis::GeometryType::Point ? tr( "Size:" ) : tr( "Line width:" ) );
  }
  mUi.mSpinBoxSize->setRange( 1, 100 );
  mUi.mSpinBoxSize->setValue( QgsSettings().value( "/Redlining/size", 1 ).toInt() );
  connect( mUi.mSpinBoxSize, qOverload<int>( &QSpinBox::valueChanged ), this, &KadasRedliningItemEditor::saveOutlineWidth );

  mUi.mToolButtonBorderColor->setAllowOpacity( true );
  mUi.mToolButtonBorderColor->setShowNoColor( true );
  mUi.mToolButtonBorderColor->setProperty( "settings_key", "outline_color" );
  QColor initialOutlineColor = QgsSymbolLayerUtils::decodeColor( QgsSettings().value( "/Redlining/outline_color", "0,0,0,255" ).toString() );
  mUi.mToolButtonBorderColor->setColor( initialOutlineColor );
  connect( mUi.mToolButtonBorderColor, &QgsColorButton::colorChanged, this, &KadasRedliningItemEditor::saveColor );

  mUi.mComboBoxOutlineStyle->setProperty( "settings_key", "outline_style" );
  mUi.mComboBoxOutlineStyle->addItem( createOutlineStyleIcon( Qt::NoPen ), QString(), QVariant::fromValue( Qt::NoPen ) );
  mUi.mComboBoxOutlineStyle->addItem( createOutlineStyleIcon( Qt::SolidLine ), QString(), QVariant::fromValue( Qt::SolidLine ) );
  mUi.mComboBoxOutlineStyle->addItem( createOutlineStyleIcon( Qt::DashLine ), QString(), QVariant::fromValue( Qt::DashLine ) );
  mUi.mComboBoxOutlineStyle->addItem( createOutlineStyleIcon( Qt::DashDotLine ), QString(), QVariant::fromValue( Qt::DashDotLine ) );
  mUi.mComboBoxOutlineStyle->addItem( createOutlineStyleIcon( Qt::DotLine ), QString(), QVariant::fromValue( Qt::DotLine ) );
  mUi.mComboBoxOutlineStyle->setCurrentIndex( QgsSettings().value( "/Redlining/outline_style", "1" ).toInt() );
  connect( mUi.mComboBoxOutlineStyle, qOverload<int>( &QComboBox::currentIndexChanged ), this, &KadasRedliningItemEditor::saveStyle );

  mUi.mToolButtonFillColor->setAllowOpacity( true );
  mUi.mToolButtonFillColor->setShowNoColor( true );
  mUi.mToolButtonFillColor->setProperty( "settings_key", "fill_color" );
  QColor initialFillColor = QgsSymbolLayerUtils::decodeColor( QgsSettings().value( "/Redlining/fill_color", "255,0,0,255" ).toString() );
  mUi.mToolButtonFillColor->setColor( initialFillColor );
  connect( mUi.mToolButtonFillColor, &QgsColorButton::colorChanged, this, &KadasRedliningItemEditor::saveColor );

  mUi.mComboBoxFillStyle->setProperty( "settings_key", "fill_style" );
  mUi.mComboBoxFillStyle->addItem( createFillStyleIcon( Qt::NoBrush ), QString(), QVariant::fromValue( Qt::NoBrush ) );
  mUi.mComboBoxFillStyle->addItem( createFillStyleIcon( Qt::SolidPattern ), QString(), QVariant::fromValue( Qt::SolidPattern ) );
  mUi.mComboBoxFillStyle->addItem( createFillStyleIcon( Qt::HorPattern ), QString(), QVariant::fromValue( Qt::HorPattern ) );
  mUi.mComboBoxFillStyle->addItem( createFillStyleIcon( Qt::VerPattern ), QString(), QVariant::fromValue( Qt::VerPattern ) );
  mUi.mComboBoxFillStyle->addItem( createFillStyleIcon( Qt::BDiagPattern ), QString(), QVariant::fromValue( Qt::BDiagPattern ) );
  mUi.mComboBoxFillStyle->addItem( createFillStyleIcon( Qt::DiagCrossPattern ), QString(), QVariant::fromValue( Qt::DiagCrossPattern ) );
  mUi.mComboBoxFillStyle->addItem( createFillStyleIcon( Qt::FDiagPattern ), QString(), QVariant::fromValue( Qt::FDiagPattern ) );
  mUi.mComboBoxFillStyle->addItem( createFillStyleIcon( Qt::CrossPattern ), QString(), QVariant::fromValue( Qt::CrossPattern ) );
  mUi.mComboBoxFillStyle->setCurrentIndex( QgsSettings().value( "/Redlining/fill_style", 1 ).toInt() );
  connect( mUi.mComboBoxFillStyle, qOverload<int>( &QComboBox::currentIndexChanged ), this, &KadasRedliningItemEditor::saveStyle );

  if ( geometryItem && geometryItem->geometryType() == Qgis::GeometryType::Line )
  {
    mUi.mLabelFillColor->setVisible( false );
    mUi.mToolButtonFillColor->setVisible( false );
    mUi.mComboBoxFillStyle->setVisible( false );
  }

  connect( this, &KadasRedliningItemEditor::styleChanged, this, &KadasRedliningItemEditor::syncWidgetToItem );

  toggleItemMeasurements( true );
}

void KadasRedliningItemEditor::setItem( KadasMapItem *item )
{
  toggleItemMeasurements( false );
  KadasMapItemEditor::setItem( item );
  toggleItemMeasurements( true );
}

void KadasRedliningItemEditor::syncItemToWidget()
{
  if ( KadasPointItem *pointItem = dynamic_cast<KadasPointItem *>( mItem ) )
  {
    mUi.mSpinBoxSize->blockSignals( true );
    mUi.mSpinBoxSize->setValue( pointItem->iconSize() );
    mUi.mSpinBoxSize->blockSignals( false );

    mUi.mToolButtonBorderColor->blockSignals( true );
    mUi.mToolButtonBorderColor->setColor( pointItem->strokeColor() );
    mUi.mToolButtonBorderColor->blockSignals( false );

    mUi.mToolButtonFillColor->blockSignals( true );
    mUi.mToolButtonFillColor->setColor( pointItem->color() );
    mUi.mToolButtonFillColor->blockSignals( false );

    mUi.mComboBoxOutlineStyle->blockSignals( true );
    mUi.mComboBoxOutlineStyle->setCurrentIndex( mUi.mComboBoxOutlineStyle->findData( QVariant::fromValue( pointItem->strokeStyle() ) ) );
    mUi.mComboBoxOutlineStyle->blockSignals( false );

    //   mUi.mComboBoxFillStyle->blockSignals( true );
    //   mUi.mComboBoxFillStyle->setCurrentIndex( mUi.mComboBoxFillStyle->findData( QVariant::fromValue( pointItem->fill().style() ) ) );
    //   mUi.mComboBoxFillStyle->blockSignals( false );
  }
  else if ( KadasGeometryItem *geometryItem = dynamic_cast<KadasGeometryItem *>( mItem ) )
  {
    mUi.mSpinBoxSize->blockSignals( true );
    mUi.mSpinBoxSize->setValue( geometryItem->outline().width() );
    mUi.mSpinBoxSize->blockSignals( false );

    mUi.mToolButtonBorderColor->blockSignals( true );
    mUi.mToolButtonBorderColor->setColor( geometryItem->outline().color() );
    mUi.mToolButtonBorderColor->blockSignals( false );

    mUi.mToolButtonFillColor->blockSignals( true );
    mUi.mToolButtonFillColor->setColor( geometryItem->fill().color() );
    mUi.mToolButtonFillColor->blockSignals( false );

    mUi.mComboBoxOutlineStyle->blockSignals( true );
    mUi.mComboBoxOutlineStyle->setCurrentIndex( mUi.mComboBoxOutlineStyle->findData( QVariant::fromValue( geometryItem->outline().style() ) ) );
    mUi.mComboBoxOutlineStyle->blockSignals( false );

    mUi.mComboBoxFillStyle->blockSignals( true );
    mUi.mComboBoxFillStyle->setCurrentIndex( mUi.mComboBoxFillStyle->findData( QVariant::fromValue( geometryItem->fill().style() ) ) );
    mUi.mComboBoxFillStyle->blockSignals( false );
  }
}

void KadasRedliningItemEditor::syncWidgetToItem()
{
  if ( KadasPointItem *pointItem = dynamic_cast<KadasPointItem *>( mItem ) )
  {
    int size = mUi.mSpinBoxSize->value();
    QColor outlineColor = mUi.mToolButtonBorderColor->color();
    QColor fillColor = mUi.mToolButtonFillColor->color();
    Qt::PenStyle lineStyle = mUi.mComboBoxOutlineStyle->currentData().value<Qt::PenStyle>();
    //Qt::BrushStyle brushStyle = static_cast<Qt::BrushStyle>( mUi.mComboBoxFillStyle->itemData( mUi.mComboBoxFillStyle->currentIndex() ).toInt() );

    pointItem->blockSignals( true );
    pointItem->setColor( fillColor );
    pointItem->setStrokeColor( outlineColor );
    pointItem->setStrokeStyle( lineStyle );
    pointItem->setIconSize( size );
    pointItem->blockSignals( false );
    emit pointItem->changed();
    emit pointItem->propertyChanged();
  }
  else if ( KadasGeometryItem *geometryItem = dynamic_cast<KadasGeometryItem *>( mItem ) )
  {
    int outlineWidth = mUi.mSpinBoxSize->value();
    QColor outlineColor = mUi.mToolButtonBorderColor->color();
    QColor fillColor = mUi.mToolButtonFillColor->color();
    Qt::PenStyle lineStyle = mUi.mComboBoxOutlineStyle->currentData().value<Qt::PenStyle>();
    Qt::BrushStyle brushStyle = static_cast<Qt::BrushStyle>( mUi.mComboBoxFillStyle->itemData( mUi.mComboBoxFillStyle->currentIndex() ).toInt() );

    geometryItem->blockSignals( true );
    geometryItem->setOutline( QPen( outlineColor, outlineWidth, lineStyle ) );
    geometryItem->setFill( QBrush( fillColor, brushStyle ) );

    geometryItem->setIconSize( 10 + 2 * outlineWidth );
    geometryItem->setIconOutline( QPen( outlineColor, outlineWidth / 4, lineStyle ) );
    geometryItem->setIconFill( QBrush( fillColor, brushStyle ) );
    geometryItem->blockSignals( false );
    emit geometryItem->changed();
    emit geometryItem->propertyChanged();
  }
}

KadasRedliningItemEditor::~KadasRedliningItemEditor()
{
  toggleItemMeasurements( false );
}

void KadasRedliningItemEditor::toggleItemMeasurements( bool enabled )
{
  KadasGeometryItem *geometryItem = qobject_cast<KadasGeometryItem *>( mItem );
  if ( !geometryItem )
  {
    return;
  }
  geometryItem->setMeasurementsEnabled( enabled );
}

void KadasRedliningItemEditor::saveColor()
{
  QgsColorButton *btn = qobject_cast<QgsColorButton *>( QObject::sender() );
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
  QComboBox *combo = qobject_cast<QComboBox *>( QObject::sender() );
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
