/***************************************************************************
    kadasredliningintergration.cpp
    ------------------------------
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

#include <QAction>
#include <QMenu>
#include <QShortcut>
#include <QSpinBox>
#include <QToolButton>

#include <qgis/qgsproject.h>
#include <qgis/qgssettings.h>
#include <qgis/qgssymbollayerutils.h>

#include <kadas/core/kadasitemlayer.h>
#include <kadas/core/mapitems/kadascircleitem.h>
#include <kadas/core/mapitems/kadaslineitem.h>
#include <kadas/core/mapitems/kadaspointitem.h>
#include <kadas/core/mapitems/kadaspolygonitem.h>
#include <kadas/core/mapitems/kadasrectangleitem.h>

#include <kadas/gui/maptools/kadasmaptoolcreateitem.h>

#include <kadas/app/kadasmainwindow.h>
#include <kadas/app/kadasredliningintergration.h>

KadasRedliningIntegration::KadasRedliningIntegration(const UI &ui, KadasMainWindow *main )
  : mUi( ui )
{
  mMainWindow = main;
  mCanvas = main->mapCanvas();

  KadasMapToolCreateItem::ItemFactory pointFactory = [=] {
    return applyStyle(new KadasPointItem(mCanvas->mapSettings().destinationCrs(), KadasPointItem::ICON_CIRCLE));
  };
  KadasMapToolCreateItem::ItemFactory squareFactory = [=] {
    return applyStyle(new KadasPointItem(mCanvas->mapSettings().destinationCrs(), KadasPointItem::ICON_FULL_BOX));
  };
  KadasMapToolCreateItem::ItemFactory triangleFactory = [=] {
    return applyStyle(new KadasPointItem(mCanvas->mapSettings().destinationCrs(), KadasPointItem::ICON_FULL_TRIANGLE));
  };
  KadasMapToolCreateItem::ItemFactory lineFactory = [=] {
    return applyStyle(new KadasLineItem(mCanvas->mapSettings().destinationCrs()));
  };
  KadasMapToolCreateItem::ItemFactory rectangleFactory = [=] {
    return applyStyle(new KadasRectangleItem(mCanvas->mapSettings().destinationCrs()));
  };
  KadasMapToolCreateItem::ItemFactory polygonFactory = [=] {
    return applyStyle(new KadasPolygonItem(mCanvas->mapSettings().destinationCrs()));
  };
  KadasMapToolCreateItem::ItemFactory circleFactory = [=] {
    return applyStyle(new KadasCircleItem(mCanvas->mapSettings().destinationCrs()));
  };


  QAction* actionNewMarker = new QAction( QIcon( ":/images/icons/redlining_point" ), tr( "Marker" ), this );

  mActionNewPoint = new QAction( QIcon( ":/images/icons/redlining_point" ), tr( "Point" ), this );
  mActionNewPoint->setCheckable( true );
  connect( mActionNewPoint, &QAction::triggered, this, [=](bool active){ toggleCreateItem(active, pointFactory); });

  mActionNewSquare = new QAction( QIcon( ":/images/icons/redlining_square" ), tr( "Square" ), this );
  mActionNewSquare->setCheckable( true );
  connect( mActionNewSquare, &QAction::triggered, this, [=](bool active){ toggleCreateItem(active, squareFactory); });

  mActionNewTriangle = new QAction( QIcon( ":/images/icons/redlining_triangle" ), tr( "Triangle" ), this );
  mActionNewTriangle->setCheckable( true );
  connect( mActionNewTriangle, &QAction::triggered, this, [=](bool active){ toggleCreateItem(active, triangleFactory); });

  mActionNewLine = new QAction( QIcon( ":/images/icons/redlining_line" ), tr( "Line" ), this );
  mActionNewLine->setCheckable( true );
  connect( mActionNewLine, &QAction::triggered, this, [=](bool active){ toggleCreateItem(active, lineFactory); });
  connect( new QShortcut( QKeySequence( Qt::CTRL + Qt::Key_D, Qt::CTRL + Qt::Key_L ), main ), &QShortcut::activated, mActionNewLine, &QAction::trigger );

  mActionNewRectangle = new QAction( QIcon( ":/images/icons/redlining_rectangle" ), tr( "Rectangle" ), this );
  mActionNewRectangle->setCheckable( true );
  connect( mActionNewRectangle, &QAction::triggered, this, [=](bool active){ toggleCreateItem(active, rectangleFactory); });
  connect( new QShortcut( QKeySequence( Qt::CTRL + Qt::Key_D, Qt::CTRL + Qt::Key_R ), main ), &QShortcut::activated, mActionNewRectangle, &QAction::trigger );

  mActionNewPolygon = new QAction( QIcon( ":/images/icons/redlining_polygon" ), tr( "Polygon" ), this );
  mActionNewPolygon->setCheckable( true );
  connect( mActionNewPolygon, &QAction::triggered, this, [=](bool active){ toggleCreateItem(active, polygonFactory); });
  connect( new QShortcut( QKeySequence( Qt::CTRL + Qt::Key_D, Qt::CTRL + Qt::Key_P ), main ), &QShortcut::activated, mActionNewPolygon, &QAction::trigger );

  mActionNewCircle = new QAction( QIcon( ":/images/icons/redlining_circle" ), tr( "Circle" ), this );
  mActionNewCircle->setCheckable( true );
  connect( mActionNewCircle, &QAction::triggered, this, [=](bool active){ toggleCreateItem(active, circleFactory); });
  connect( new QShortcut( QKeySequence( Qt::CTRL + Qt::Key_D, Qt::CTRL + Qt::Key_C ), main ), &QShortcut::activated, mActionNewCircle, &QAction::trigger );

  // TODO
//  mActionNewText = new QAction( QIcon( ":/images/icons/redlining_text" ), tr( "Text" ), this );
//  mActionNewText->setCheckable( true );
//  connect( mActionNewText, &QAction::triggered, this, &KadasRedliningIntegration::setTextTool );
//  connect( new QShortcut( QKeySequence( Qt::CTRL + Qt::Key_D, Qt::CTRL + Qt::Key_T ), main ), &QShortcut::activated, mActionNewText, &QAction::trigger );

  mUi.buttonNewObject->setToolTip( tr( "New Object" ) );
  QMenu* menuNewMarker = new QMenu();
  menuNewMarker->addAction( mActionNewPoint );
  menuNewMarker->addAction( mActionNewSquare );
  menuNewMarker->addAction( mActionNewTriangle );
  actionNewMarker->setMenu( menuNewMarker );
  QMenu* menuNewObject = new QMenu();
  menuNewObject->addAction( actionNewMarker );
  menuNewObject->addAction( mActionNewLine );
  menuNewObject->addAction( mActionNewRectangle );
  menuNewObject->addAction( mActionNewPolygon );
  menuNewObject->addAction( mActionNewCircle );
  menuNewObject->addAction( mActionNewText );
  mUi.buttonNewObject->setMenu( menuNewObject );
  mUi.buttonNewObject->setPopupMode( QToolButton::MenuButtonPopup );
  mUi.buttonNewObject->setDefaultAction( mActionNewPoint );

  mUi.spinBoxSize->setRange( 1, 100 );
  mUi.spinBoxSize->setValue( QgsSettings().value( "/Redlining/size", 1 ).toInt() );
  connect( mUi.spinBoxSize, qOverload<int>(&QSpinBox::valueChanged), this, &KadasRedliningIntegration::saveOutlineWidth );

  mUi.colorButtonOutlineColor->setAllowOpacity( true );
  mUi.colorButtonOutlineColor->setProperty( "settings_key", "outline_color" );
  QColor initialOutlineColor = QgsSymbolLayerUtils::decodeColor( QgsSettings().value( "/Redlining/outline_color", "0,0,0,255" ).toString() );
  mUi.colorButtonOutlineColor->setColor( initialOutlineColor );
  connect( mUi.colorButtonOutlineColor, &QgsColorButton::colorChanged, this, &KadasRedliningIntegration::saveColor );

  mUi.comboOutlineStyle->setProperty( "settings_key", "outline_style" );
  mUi.comboOutlineStyle->addItem( createOutlineStyleIcon( Qt::NoPen ), QString(), static_cast<int>( Qt::NoPen ) );
  mUi.comboOutlineStyle->addItem( createOutlineStyleIcon( Qt::SolidLine ), QString(), static_cast<int>( Qt::SolidLine ) );
  mUi.comboOutlineStyle->addItem( createOutlineStyleIcon( Qt::DashLine ), QString(), static_cast<int>( Qt::DashLine ) );
  mUi.comboOutlineStyle->addItem( createOutlineStyleIcon( Qt::DashDotLine ), QString(), static_cast<int>( Qt::DashDotLine ) );
  mUi.comboOutlineStyle->addItem( createOutlineStyleIcon( Qt::DotLine ), QString(), static_cast<int>( Qt::DotLine ) );
  mUi.comboOutlineStyle->setCurrentIndex( QgsSettings().value( "/Redlining/outline_style", "1" ).toInt() );
  connect( mUi.comboOutlineStyle, qOverload<int>(&QComboBox::currentIndexChanged), this, &KadasRedliningIntegration::saveStyle );

  mUi.colorButtonFillColor->setAllowOpacity( true );
  mUi.colorButtonFillColor->setProperty( "settings_key", "fill_color" );
  QColor initialFillColor = QgsSymbolLayerUtils::decodeColor( QgsSettings().value( "/Redlining/fill_color", "255,0,0,255" ).toString() );
  mUi.colorButtonFillColor->setColor( initialFillColor );
  connect( mUi.colorButtonFillColor, &QgsColorButton::colorChanged, this, &KadasRedliningIntegration::saveColor );

  mUi.comboFillStyle->setProperty( "settings_key", "fill_style" );
  mUi.comboFillStyle->addItem( createFillStyleIcon( Qt::NoBrush ), QString(), static_cast<int>( Qt::NoBrush ) );
  mUi.comboFillStyle->addItem( createFillStyleIcon( Qt::SolidPattern ), QString(), static_cast<int>( Qt::SolidPattern ) );
  mUi.comboFillStyle->addItem( createFillStyleIcon( Qt::HorPattern ), QString(), static_cast<int>( Qt::HorPattern ) );
  mUi.comboFillStyle->addItem( createFillStyleIcon( Qt::VerPattern ), QString(), static_cast<int>( Qt::VerPattern ) );
  mUi.comboFillStyle->addItem( createFillStyleIcon( Qt::BDiagPattern ), QString(), static_cast<int>( Qt::BDiagPattern ) );
  mUi.comboFillStyle->addItem( createFillStyleIcon( Qt::DiagCrossPattern ), QString(), static_cast<int>( Qt::DiagCrossPattern ) );
  mUi.comboFillStyle->addItem( createFillStyleIcon( Qt::FDiagPattern ), QString(), static_cast<int>( Qt::FDiagPattern ) );
  mUi.comboFillStyle->addItem( createFillStyleIcon( Qt::CrossPattern ), QString(), static_cast<int>( Qt::CrossPattern ) );
  mUi.comboFillStyle->setCurrentIndex( QgsSettings().value( "/Redlining/fill_style", "1" ).toInt() );
  connect( mUi.comboFillStyle, qOverload<int>(&QComboBox::currentIndexChanged), this, &KadasRedliningIntegration::saveStyle );

  connect( this, &KadasRedliningIntegration::styleChanged, this, &KadasRedliningIntegration::updateItemStyle );
}

KadasItemLayer* KadasRedliningIntegration::getOrCreateLayer()
{
  if(!getLayer()) {
    KadasItemLayer* layer = new KadasItemLayer( tr( "Redlining" ) );
    mLayerId = layer->id();
    QgsProject::instance()->addMapLayer( layer );
  }
  return getLayer();
}

KadasItemLayer* KadasRedliningIntegration::getLayer() const
{
  return qobject_cast<KadasItemLayer*>(QgsProject::instance()->mapLayer(mLayerId));
}

void KadasRedliningIntegration::toggleCreateItem(bool active, const std::function<KadasMapItem*()>& itemFactory)
{
  QAction* action = qobject_cast<QAction*>(QObject::sender());
  if ( active )
  {
    KadasMapToolCreateItem* tool = new KadasMapToolCreateItem(mCanvas, itemFactory, getOrCreateLayer());
    tool->setShowInputWidget(QSettings().value( "/kadas/showNumericInput", false ).toBool());
    tool->setAction( action );
    connect( tool, &QgsMapTool::deactivated, tool, &QObject::deleteLater );
    mUi.buttonNewObject->setDefaultAction( action );
    mMainWindow->layerTreeView()->setCurrentLayer( getOrCreateLayer() );
    mMainWindow->layerTreeView()->setLayerVisible( getOrCreateLayer(), true );
    mCanvas->setMapTool( tool );
  }
  else if ( !active && mCanvas->mapTool() && mCanvas->mapTool()->action() == action )
  {
    mCanvas->unsetMapTool( mCanvas->mapTool() );
  }
}

KadasGeometryItem* KadasRedliningIntegration::applyStyle(KadasGeometryItem *item)
{
  int outlineWidth = mUi.spinBoxSize->value();
  QColor outlineColor = mUi.colorButtonOutlineColor->color();
  QColor fillColor = mUi.colorButtonFillColor->color();
  Qt::PenStyle lineStyle = static_cast<Qt::PenStyle>( mUi.comboOutlineStyle->itemData( mUi.comboOutlineStyle->currentIndex() ).toInt() );
  Qt::BrushStyle brushStyle = static_cast<Qt::BrushStyle>( mUi.comboFillStyle->itemData( mUi.comboFillStyle->currentIndex() ).toInt() );

  item->setOutlineWidth(outlineWidth);
  item->setOutlineColor(outlineColor);
  item->setFillColor(fillColor);
  item->setLineStyle(lineStyle);
  item->setBrushStyle(brushStyle);

  item->setIconOutlineWidth(outlineWidth / 4);
  item->setIconSize(2 * outlineWidth);
  item->setIconOutlineColor(outlineColor);
  item->setIconFillColor(fillColor);
  item->setIconLineStyle(lineStyle);
  item->setIconBrushStyle(brushStyle);

  return item;
}

void KadasRedliningIntegration::saveColor()
{
  QgsColorButton* btn = qobject_cast<QgsColorButton*>( QObject::sender() );
  QString key = QString( "/Redlining/" ) + btn->property( "settings_key" ).toString();
  QgsSettings().setValue( key, QgsSymbolLayerUtils::encodeColor( btn->color() ) );
  emit styleChanged();
}

void KadasRedliningIntegration::saveOutlineWidth()
{
  QgsSettings().setValue( "/Redlining/size", mUi.spinBoxSize->value() );
  emit styleChanged();
}

void KadasRedliningIntegration::saveStyle()
{
  QComboBox* combo = qobject_cast<QComboBox*>( QObject::sender() );
  QString key = QString( "/Redlining/" ) + combo->property( "settings_key" ).toString();
  QgsSettings().setValue( key, combo->currentIndex() );
  emit styleChanged();
}

void KadasRedliningIntegration::updateItemStyle()
{
  KadasMapToolCreateItem* tool = qobject_cast<KadasMapToolCreateItem*>(mCanvas->mapTool());
  if(!tool) {
    return;
  }
  KadasGeometryItem* item = qobject_cast<KadasGeometryItem*>(tool->currentItem());
  if(!item) {
    return;
  }
  applyStyle(item);
}

QIcon KadasRedliningIntegration::createOutlineStyleIcon( Qt::PenStyle style )
{
  QPixmap pixmap( 16, 16 );
  pixmap.fill( Qt::transparent );
  QPainter painter( &pixmap );
  painter.setRenderHint( QPainter::Antialiasing );
  QPen pen;
  pen.setStyle( style );
  pen.setColor( QColor( 232, 244, 255 ) );
  pen.setWidth( 2 );
  painter.setPen( pen );
  painter.drawLine( 0, 8, 16, 8 );
  return pixmap;
}

QIcon KadasRedliningIntegration::createFillStyleIcon( Qt::BrushStyle style )
{
  QPixmap pixmap( 16, 16 );
  pixmap.fill( Qt::transparent );
  QPainter painter( &pixmap );
  painter.setRenderHint( QPainter::Antialiasing );
  QBrush brush;
  brush.setStyle( style );
  brush.setColor( QColor( 232, 244, 255 ) );
  painter.fillRect( 0, 0, 16, 16, brush );
  return pixmap;
}
