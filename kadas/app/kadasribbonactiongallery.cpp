/***************************************************************************
    kadasribbonactiongallery.cpp
    ----------------------------
    copyright            : (C) 2026 by OPENGIS.ch
    email                : info@opengis.ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <algorithm>

#include <QAction>
#include <QFrame>
#include <QGridLayout>
#include <QIcon>
#include <QLabel>
#include <QMenu>
#include <QPainter>
#include <QPixmap>
#include <QToolButton>

#include "kadasribbonactiongallery.h"


namespace
{
  //! Returns \a src tinted to \a color (keeping its alpha shape), in device
  //! pixels with a 1.0 device-pixel ratio so offsets stay pixel-exact.
  QPixmap tinted( const QPixmap &src, const QColor &color )
  {
    QPixmap base = src;
    base.setDevicePixelRatio( 1.0 );
    QPixmap result( base.size() );
    result.fill( Qt::transparent );
    QPainter p( &result );
    p.drawPixmap( 0, 0, base );
    p.setCompositionMode( QPainter::CompositionMode_SourceIn );
    p.fillRect( result.rect(), color );
    p.end();
    return result;
  }

  //! Builds a monochrome icon tinted \a fill, optionally wrapped in a 1px black
  //! \a outline halo to keep the active (yellow) icon legible.
  QPixmap tintedIcon( const QPixmap &src, const QColor &fill, bool outline )
  {
    if ( src.isNull() )
      return src;
    const QPixmap fillPix = tinted( src, fill );
    QPixmap result( fillPix.size() );
    result.fill( Qt::transparent );
    QPainter p( &result );
    if ( outline )
    {
      const QPixmap outlinePix = tinted( src, QColor( 0, 0, 0 ) );
      for ( int dx = -1; dx <= 1; ++dx )
        for ( int dy = -1; dy <= 1; ++dy )
          if ( dx != 0 || dy != 0 )
            p.drawPixmap( dx, dy, outlinePix );
    }
    p.drawPixmap( 0, 0, fillPix );
    p.end();
    result.setDevicePixelRatio( src.devicePixelRatio() );
    return result;
  }
} // namespace


KadasRibbonActionGallery::KadasRibbonActionGallery( int columns, QWidget *parent )
  : QWidget( parent )
  , mColumns( std::max( 1, columns ) )
{
  mGrid = new QGridLayout( this );
  mGrid->setContentsMargins( 6, 6, 6, 6 );
  mGrid->setHorizontalSpacing( 2 );
  mGrid->setVerticalSpacing( 2 );
}

void KadasRibbonActionGallery::addSection( const QString &title )
{
  finishPartialRow();

  if ( mRow > 0 )
  {
    QFrame *separator = new QFrame( this );
    separator->setFrameShape( QFrame::HLine );
    separator->setFrameShadow( QFrame::Sunken );
    mGrid->addWidget( separator, mRow++, 0, 1, mColumns );
  }

  QLabel *label = new QLabel( title, this );
  QFont labelFont = label->font();
  labelFont.setBold( true );
  label->setFont( labelFont );
  mGrid->addWidget( label, mRow++, 0, 1, mColumns );
}

void KadasRibbonActionGallery::addActionTile( QAction *action )
{
  QToolButton *tile = new QToolButton( this );
  // Note: deliberately NOT using setDefaultAction(): it re-copies the action's
  // raw icon onto the button on every QAction::changed() (e.g. each setChecked),
  // which would clobber the custom white/yellow icon below. We mirror the
  // action manually instead.
  tile->setToolButtonStyle( Qt::ToolButtonTextUnderIcon );
  tile->setIconSize( mTileIconSize );
  tile->setFixedSize( mTileSize );
  tile->setText( action->text() );
  tile->setToolTip( action->text() );
  tile->setCheckable( action->isCheckable() );
  tile->setChecked( action->isChecked() );
  tile->setEnabled( action->isEnabled() );

  // Monochrome tile icon: white while idle, brand yellow with a black outline
  // when the tool is active (the tile mirrors the checkable action's state).
  const QPixmap base = action->icon().pixmap( mTileIconSize, tile->devicePixelRatioF() );
  QIcon tileIcon;
  tileIcon.addPixmap( tintedIcon( base, QColor( 255, 255, 255 ), false ), QIcon::Normal, QIcon::Off );
  tileIcon.addPixmap( tintedIcon( base, QColor( 0xFF, 0xCC, 0x00 ), true ), QIcon::Normal, QIcon::On );
  tile->setIcon( tileIcon );

  // Clicking the tile runs the underlying action; the tile's highlight follows
  // the action's checked state so it clears when the tool is deactivated or a
  // different tool is selected (the actions share an exclusive group).
  // A one-shot action carrying a sub-menu (e.g. Image → From file / From URL)
  // pops that menu in place instead, so its options stay one level deep.
  if ( action->menu() )
  {
    tile->setMenu( action->menu() );
    tile->setPopupMode( QToolButton::InstantPopup );
    // Picking any sub-option (even when its dialog is cancelled) makes this the
    // category's current tool, so forward each sub-action as this action.
    const QList<QAction *> subActions = action->menu()->actions();
    for ( QAction *sub : subActions )
      connect( sub, &QAction::triggered, this, [this, action] { emit actionTriggered( action ); } );
  }
  else
  {
    connect( tile, &QToolButton::clicked, action, [action] { action->trigger(); } );
  }
  connect( action, &QAction::toggled, tile, &QToolButton::setChecked );
  connect( action, &QAction::changed, tile, [tile, action] { tile->setEnabled( action->isEnabled() ); } );
  connect( action, &QAction::triggered, this, [this, action] { emit actionTriggered( action ); } );

  mGrid->addWidget( tile, mRow, mCol );
  if ( ++mCol >= mColumns )
  {
    mCol = 0;
    ++mRow;
  }
}

void KadasRibbonActionGallery::finishPartialRow()
{
  if ( mCol != 0 )
  {
    mCol = 0;
    ++mRow;
  }
}
