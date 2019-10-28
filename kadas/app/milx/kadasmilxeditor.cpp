/***************************************************************************
    kadasmilxeditor.cpp
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

#include <QHBoxLayout>
#include <QLabel>
#include <QToolButton>

#include <kadas/app/milx/kadasmilxeditor.h>
#include <kadas/app/milx/kadasmilxitem.h>
#include <kadas/app/milx/kadasmilxlibrary.h>

KadasMilxEditor::KadasMilxEditor( KadasMapItem *item, EditorType type, KadasMilxLibrary *library, QWidget *parent )
  : KadasMapItemEditor( item, parent )
  , mLibrary( library )
{
  if ( type == KadasMapItemEditor::CreateItemEditor )
  {
    setLayout( new QHBoxLayout() );

    layout()->addWidget( new QLabel( tr( "Symbol:" ) ) );
    layout()->setSpacing( 2 );
    layout()->setMargin( 0 );

    mSymbolButton = new QToolButton();
    mSymbolButton->setText( tr( "Select..." ) );
    mSymbolButton->setIconSize( QSize( 32, 32 ) );
    mSymbolButton->setCheckable( true );
    mSymbolButton->setFixedHeight( 35 );
    layout()->addWidget( mSymbolButton );

    connect( mSymbolButton, &QToolButton::clicked, this, &KadasMilxEditor::toggleLibrary );
    connect( mLibrary, &KadasMilxLibrary::symbolSelected, this, &KadasMilxEditor::symbolSelected );
    connect( mLibrary, &KadasMilxLibrary::visibilityChanged, mSymbolButton, &QToolButton::setChecked );
  }
}

void KadasMilxEditor::syncItemToWidget()
{
  // TODO
}

void KadasMilxEditor::syncWidgetToItem()
{
  if ( dynamic_cast<KadasMilxItem *>( mItem ) )
  {
    static_cast<KadasMilxItem *>( mItem )->setSymbol( mSelectedSymbol );
  }
}

void KadasMilxEditor::toggleLibrary( bool enabled )
{
  if ( enabled )
  {
    double width = 240;
    double height = 320;
    QPoint anchor = mSymbolButton->mapToGlobal( QPoint( 0.5 * mSymbolButton->width(), 0 ) );
    mLibrary->resize( width, height );
    mLibrary->move( anchor.x() - 0.5 * width, anchor.y() - height );
    mLibrary->show();
    mLibrary->focusFilter();
  }
}

void KadasMilxEditor::symbolSelected( const KadasMilxClient::SymbolDesc &symbolTemplate )
{
  if ( !symbolTemplate.symbolXml.isEmpty() )
  {
    mSymbolButton->setIcon( QIcon( QPixmap::fromImage( symbolTemplate.icon ) ) );
    mSymbolButton->setText( "" );
  }
  else
  {
    mSymbolButton->setIcon( QIcon() );
    mSymbolButton->setText( tr( "Select..." ) );
  }
  mSelectedSymbol = symbolTemplate;
  if ( dynamic_cast<KadasMilxItem *>( mItem ) && mItem->constState()->drawStatus == KadasMapItem::State::Empty )
  {
    static_cast<KadasMilxItem *>( mItem )->setSymbol( symbolTemplate );
  }
}
