/***************************************************************************
    kadasribbonsplitbutton.cpp
    --------------------------
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

#include <QAction>
#include <QMenu>
#include <QPoint>
#include <QToolButton>
#include <QWidgetAction>

#include <qgis/qgssettingsentryimpl.h>

#include "kadasribbonactiongallery.h"
#include "kadasribbonsplitbutton.h"


KadasRibbonSplitButton::KadasRibbonSplitButton( QToolButton *button, const QString &categoryLabel, const QIcon &fallbackIcon, const QgsSettingsEntryString *lastToolSetting, QObject *parent )
  : QObject( parent )
  , mButton( button )
  , mCategoryLabel( categoryLabel )
  , mFallbackIcon( fallbackIcon )
  , mLastToolSetting( lastToolSetting )
{}

void KadasRibbonSplitButton::addAction( QAction *action )
{
  mActions.append( action );

  // Keep the button's pressed look and current tool in sync with the tool
  // action's checked state (driven by the shared exclusive action group).
  if ( action->isCheckable() )
  {
    connect( action, &QAction::toggled, this, [this, action]( bool on ) {
      if ( on )
      {
        setCurrentAction( action, true );
        mButton->setChecked( true );
      }
      else if ( action == mCurrentAction )
      {
        mButton->setChecked( false );
      }
    } );
  }
}

void KadasRibbonSplitButton::finish()
{
  mMenu = new QMenu( mButton );
  mMenu->setObjectName( QStringLiteral( "mRibbonGalleryMenu" ) );
  mGallery = new KadasRibbonActionGallery( 4, mMenu );
  for ( QAction *action : std::as_const( mActions ) )
  {
    mGallery->addActionTile( action );
  }
  QWidgetAction *galleryAction = new QWidgetAction( mMenu );
  galleryAction->setDefaultWidget( mGallery );
  mMenu->addAction( galleryAction );
  connect( mGallery, &KadasRibbonActionGallery::actionTriggered, this, [this]( QAction *action ) {
    // The picked tool becomes the category's current tool. One-shot commands
    // (non-checkable, e.g. Image) are shown as current but not left pressed, so
    // they don't keep a stale active highlight.
    setCurrentAction( action, action->isCheckable() );
    if ( !action->isCheckable() )
      mButton->setChecked( false );
    mMenu->close();
  } );

  mButton->setMenu( mMenu );
  mButton->setPopupMode( QToolButton::MenuButtonPopup );
  mButton->setCheckable( true );
  mButton->setText( mCategoryLabel );

  // Clicking the main (non-arrow) area re-runs the remembered tool. If the
  // current tool carries a sub-menu (e.g. Image), re-open that menu instead.
  connect( mButton, &QToolButton::clicked, this, [this] {
    if ( !mCurrentAction )
      return;
    if ( mCurrentAction->menu() )
    {
      mCurrentAction->menu()->exec( mButton->mapToGlobal( QPoint( 0, mButton->height() ) ) );
      return;
    }
    mCurrentAction->trigger();
    if ( !mCurrentAction->isCheckable() )
      mButton->setChecked( false );
  } );

  // Restore the last-used tool (or fall back to the first checkable one). Only
  // checkable tools can be the remembered current tool; one-shot commands like
  // Image stay transient.
  QAction *restore = nullptr;
  const QString saved = mLastToolSetting ? mLastToolSetting->value() : QString();
  if ( !saved.isEmpty() )
  {
    for ( QAction *action : std::as_const( mActions ) )
    {
      if ( action->isCheckable() && action->objectName() == saved )
      {
        restore = action;
        break;
      }
    }
  }
  if ( !restore )
  {
    for ( QAction *action : std::as_const( mActions ) )
    {
      if ( action->isCheckable() )
      {
        restore = action;
        break;
      }
    }
  }
  if ( restore )
  {
    setCurrentAction( restore, false );
  }
}

void KadasRibbonSplitButton::setCurrentAction( QAction *action, bool persist )
{
  mCurrentAction = action;
  mButton->setIcon( action->icon().isNull() ? mFallbackIcon : action->icon() );
  // The button label shows the active tool's name (e.g. "Point", "Diamond"),
  // not the category, so the user always sees what a click will do.
  mButton->setText( action->text() );
  mButton->setToolTip( action->text() );
  if ( persist && mLastToolSetting && !action->objectName().isEmpty() )
  {
    mLastToolSetting->setValue( action->objectName() );
  }
}
