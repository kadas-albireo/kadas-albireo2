/***************************************************************************
    kadassidepanel.cpp
    ------------------
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

#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

#include <qgis/qgsmapcanvas.h>

#include "kadas/gui/kadassidepanel.h"

KadasSidePanel::KadasSidePanel( QgsMapCanvas *canvas, KadasSidePanelHost::Edge edge )
  : QFrame( nullptr )
  , mCanvas( canvas )
  , mEdge( edge )
{
  setObjectName( QStringLiteral( "KadasSidePanel" ) );
  setStyleSheet( QStringLiteral( "QFrame#KadasSidePanel { background-color: palette(window); border: 1px solid palette(mid); border-radius: 4px; }" ) );
  setCursor( Qt::ArrowCursor );
  // Fill the full vertical space of the host so the panel background is
  // continuous; content is pinned to the top by a trailing stretch.
  setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Expanding );

  mHost = findHost();
  if ( mHost )
    mHost->addPanel( this );
}

KadasSidePanel::~KadasSidePanel()
{
  if ( mHost )
    mHost->removePanel( this );
}

QVBoxLayout *KadasSidePanel::ensureOuterLayout()
{
  if ( !mOuterLayout )
  {
    mOuterLayout = new QVBoxLayout( this );
    mOuterLayout->setContentsMargins( 8, 4, 8, 4 );
    mOuterLayout->setSpacing( 4 );
    // Trailing stretch keeps content pinned to the top while the panel itself
    // expands to fill the whole vertical space of the host.
    mOuterLayout->addStretch( 1 );
  }
  return mOuterLayout;
}

QFormLayout *KadasSidePanel::ensureForm()
{
  if ( !mFormLayout )
  {
    mFormLayout = new QFormLayout();
    mFormLayout->setContentsMargins( 0, 0, 0, 0 );
    mFormLayout->setSpacing( 4 );
    mFormLayout->setFieldGrowthPolicy( QFormLayout::AllNonFixedFieldsGrow );
    // Insert before the trailing stretch so content stays pinned to the top.
    QVBoxLayout *outer = ensureOuterLayout();
    outer->insertLayout( outer->count() - 1, mFormLayout );
  }
  return mFormLayout;
}

void KadasSidePanel::setTitle( const QString &title )
{
  if ( !mHeaderRow )
  {
    mHeaderRow = new QHBoxLayout();
    mHeaderRow->setContentsMargins( 0, 0, 0, 0 );
    mHeaderRow->setSpacing( 4 );

    mTitleLabel = new QLabel();
    QFont font = mTitleLabel->font();
    font.setBold( true );
    mTitleLabel->setFont( font );
    mHeaderRow->addWidget( mTitleLabel, 1 );

    QPushButton *closeButton = new QPushButton();
    closeButton->setIcon( QIcon( QStringLiteral( ":/kadas/icons/close" ) ) );
    closeButton->setToolTip( tr( "Close" ) );
    connect( closeButton, &QPushButton::clicked, this, &KadasSidePanel::closeRequested );
    mHeaderRow->addWidget( closeButton, 0 );

    ensureOuterLayout()->insertLayout( 0, mHeaderRow );
  }
  mTitleLabel->setText( title );
}

void KadasSidePanel::addUndoRedoRow()
{
  if ( mUndoRedoRow )
    return;

  mUndoRedoRow = new QHBoxLayout();
  mUndoRedoRow->setContentsMargins( 0, 0, 0, 0 );
  mUndoRedoRow->setSpacing( 4 );

  mUndoButton = new QPushButton();
  mUndoButton->setIcon( QIcon( QStringLiteral( ":/kadas/icons/undo" ) ) );
  mUndoButton->setToolTip( tr( "Undo" ) );
  mUndoButton->setEnabled( false );
  connect( mUndoButton, &QPushButton::clicked, this, &KadasSidePanel::undoRequested );
  mUndoRedoRow->addWidget( mUndoButton );

  mRedoButton = new QPushButton();
  mRedoButton->setIcon( QIcon( QStringLiteral( ":/kadas/icons/redo" ) ) );
  mRedoButton->setToolTip( tr( "Redo" ) );
  mRedoButton->setEnabled( false );
  connect( mRedoButton, &QPushButton::clicked, this, &KadasSidePanel::redoRequested );
  mUndoRedoRow->addWidget( mRedoButton );

  mUndoRedoRow->addStretch( 1 );

  // Insert right after the header row (if any), before any content.
  ensureOuterLayout()->insertLayout( mHeaderRow ? 1 : 0, mUndoRedoRow );
}

void KadasSidePanel::setCanUndo( bool enabled )
{
  if ( mUndoButton )
    mUndoButton->setEnabled( enabled );
}

void KadasSidePanel::setCanRedo( bool enabled )
{
  if ( mRedoButton )
    mRedoButton->setEnabled( enabled );
}

void KadasSidePanel::addRow( const QString &label, QWidget *widget )
{
  ensureForm()->addRow( label, widget );
}

void KadasSidePanel::addRow( const QString &label, const QList<QWidget *> &widgets )
{
  QHBoxLayout *row = new QHBoxLayout();
  row->setContentsMargins( 0, 0, 0, 0 );
  row->setSpacing( 4 );
  for ( QWidget *widget : widgets )
    row->addWidget( widget );
  ensureForm()->addRow( label, row );
}

void KadasSidePanel::addRow( QWidget *widget )
{
  ensureForm()->addRow( widget );
}

void KadasSidePanel::addRow( QLayout *layout )
{
  ensureForm()->addRow( layout );
}

QFormLayout *KadasSidePanel::addGroup( const QString &title )
{
  QGroupBox *box = new QGroupBox( title );
  QFormLayout *form = new QFormLayout( box );
  form->setFieldGrowthPolicy( QFormLayout::AllNonFixedFieldsGrow );
  ensureForm()->addRow( box );
  return form;
}

KadasSidePanelHost *KadasSidePanel::findHost() const
{
  if ( !mCanvas )
    return nullptr;
  QWidget *window = mCanvas->window();
  if ( !window )
    return nullptr;
  return window->findChild<KadasSidePanelHost *>( KadasSidePanelHost::objectNameForEdge( mEdge ) );
}
