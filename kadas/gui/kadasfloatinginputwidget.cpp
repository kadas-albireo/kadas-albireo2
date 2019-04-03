/***************************************************************************
    kadasfloatinginputwidget.cpp
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
#include <QLabel>
#include <QKeyEvent>

#include <qgis/qgsmapcanvas.h>

#include "kadasfloatinginputwidget.h"

KadasFloatingInputWidgetField::KadasFloatingInputWidgetField( QValidator* validator, QWidget* parent ) : QLineEdit( parent )
{
  setValidator( validator );
  connect( this, &KadasFloatingInputWidgetField::returnPressed, this, &KadasFloatingInputWidgetField::checkInputChanged );
}

void KadasFloatingInputWidgetField::setText( const QString& text )
{
  QLineEdit::setText( text );
  mPrevText = text;
}

void KadasFloatingInputWidgetField::focusOutEvent( QFocusEvent *ev )
{
  QString curText = text();
  if ( curText != mPrevText )
  {
    mPrevText = curText;
    emit inputChanged();
  }
  QLineEdit::focusOutEvent( ev );
}

void KadasFloatingInputWidgetField::checkInputChanged()
{
  QString curText = text();
  if ( curText != mPrevText )
  {
    mPrevText = curText;
    emit inputChanged();
  }
  else
  {
    emit inputConfirmed();
  }
}

///////////////////////////////////////////////////////////////////////////////

KadasFloatingInputWidget::KadasFloatingInputWidget( QgsMapCanvas *canvas )
    : QWidget( canvas ), mCanvas( canvas ), mFocusedInput( 0 )
{
  setObjectName( "FloatingInputWidget" );
  QGridLayout* gridLayout = new QGridLayout();
  gridLayout->setContentsMargins( 2, 2, 2, 2 );
  gridLayout->setSpacing( 1 );
  gridLayout->setSizeConstraint( QLayout::SetFixedSize );
  setLayout( gridLayout );
  // Initially out of sight
  move( -1000, -1000 );
}

int KadasFloatingInputWidget::addInputField( const QString &label, KadasFloatingInputWidgetField* widget, bool initiallyfocused )
{
  QGridLayout* gridLayout = static_cast<QGridLayout*>( layout() );
  int row = gridLayout->isEmpty() ? 0 : gridLayout->rowCount();
  gridLayout->addWidget( new QLabel( label ), row, 0, 1, 1 );;
  gridLayout->addWidget( widget, row, 1, 1, 1 );
  mInputFields.append( widget );
  if ( initiallyfocused )
  {
    setFocusedInputField( widget );
    mInitiallyFocusedInput = row;
  }
  return row;
}

void KadasFloatingInputWidget::removeInputField( int idx )
{
  if ( mFocusedInput == mInputFields.at( idx ) )
  {
    int n = mInputFields.size();
    int nextIdx = ( n + mInputFields.indexOf( mFocusedInput ) - 1 ) % n;
    for ( int i = 0; i < n && mInputFields[nextIdx]->isHidden(); ++i )
    {
      nextIdx = ( nextIdx - 1 ) % n;
    }
    setFocusedInputField( mInputFields[nextIdx] );
  }
  mInputFields.removeAt( idx );
  if ( mInitiallyFocusedInput >= idx )
  {
    mInitiallyFocusedInput = qMax( 0, mInitiallyFocusedInput - 1 );
  }
  // Because re-creating the layout is actually easier than just deleting a row...
  QGridLayout* oldLayout = static_cast<QGridLayout*>( layout() );
  QGridLayout* newLayout = new QGridLayout();
  newLayout->setContentsMargins( 2, 2, 2, 2 );
  newLayout->setSpacing( 1 );
  newLayout->setSizeConstraint( QLayout::SetFixedSize );
  for ( int row = 0, newrow = 0, n = oldLayout->rowCount(); row < n; ++row )
  {
    QLayoutItem* item0 = oldLayout->itemAtPosition( row, 0 );
    QLayoutItem* item1 = oldLayout->itemAtPosition( row, 1 );
    if ( row != idx )
    {
      if ( item0 && item1 )
      {
        oldLayout->removeItem( item0 );
        oldLayout->removeItem( item1 );
        newLayout->addWidget( item0->widget(), newrow, 0 );
        newLayout->addWidget( item1->widget(), newrow, 1 );
        ++newrow;
      }
    }
    else
    {
      delete item0->widget();
      delete item1->widget();
    }
  }
  delete oldLayout;
  setLayout( newLayout );
}

void KadasFloatingInputWidget::setInputFieldVisible( int idx, bool visible )
{
  QGridLayout* gridLayout = static_cast<QGridLayout*>( layout() );
  if ( idx >= 0 && idx < gridLayout->rowCount() )
  {
    gridLayout->itemAtPosition( idx, 0 )->widget()->setVisible( visible );
    gridLayout->itemAtPosition( idx, 1 )->widget()->setVisible( visible );
  }
  if ( !visible && mInputFields[idx] == mFocusedInput )
  {
    int n = mInputFields.size();
    int nextIdx = ( idx + 1 ) % n;
    for ( int i = 0; i < n && mInputFields[nextIdx]->isHidden(); ++i )
    {
      nextIdx = ( nextIdx + 1 ) % n;
    }
    setFocusedInputField( mInputFields[nextIdx] );
  }
}

void KadasFloatingInputWidget::setFocusedInputField( KadasFloatingInputWidgetField* widget )
{
  if ( mFocusedInput )
  {
    mFocusedInput->removeEventFilter( this );
  }
  mFocusedInput = widget;
  if ( !mFocusedInput )
  {
    return;
  }
  if ( mFocusedInput->isVisible() )
  {
    mFocusedInput->setFocus();
    mFocusedInput->selectAll();
  }
  mFocusedInput->installEventFilter( this );
}

bool KadasFloatingInputWidget::eventFilter( QObject *obj, QEvent *ev )
{
  // If currently focused widget loses focus, make it receive focus again
  if ( obj == mFocusedInput && mFocusedInput->isVisible() && ev->type() == QEvent::FocusOut )
  {
    mFocusedInput->setFocus();
    mFocusedInput->selectAll();
    return true;
  }
  return QWidget::eventFilter( obj, ev );
}

void KadasFloatingInputWidget::adjustCursorAndExtent( const QgsPointXY& geoPos )
{
  // If position is not within visible extent, center map there
  if ( !mCanvas->mapSettings().visibleExtent().contains( geoPos ) )
  {
    QgsRectangle rect = mCanvas->mapSettings().visibleExtent();
    rect = QgsRectangle( geoPos.x() - 0.5 * rect.width(), geoPos.y() - 0.5 * rect.height(), geoPos.x() + 0.5 * rect.width(), geoPos.y() + 0.5 * rect.height() );
    mCanvas->setExtent( rect );
    mCanvas->refresh();
  }
  // Then, move cursor to corresponding screen position and simulate move event
  double x = geoPos.x(), y = geoPos.y();
  mCanvas->getCoordinateTransform()->transformInPlace( x, y );
  QPoint p( qRound( x ), qRound( y ) );
  QCursor::setPos( mCanvas->mapToGlobal( p ) );
  move( p.x(), p.y() + 20 );
}

bool KadasFloatingInputWidget::focusNextPrevChild( bool /*next*/ )
{
  // Disable automatic TAB event handling
  // http://stackoverflow.com/a/21351638/1338788
  return false;
}

void KadasFloatingInputWidget::keyPressEvent( QKeyEvent *ev )
{
  // Override tab handling to ensure only the input fields inside the widget receive focus
  if ( ev->key() == Qt::Key_Tab || ev->key() == Qt::Key_Down )
  {
    int n = mInputFields.size();
    int nextIdx = ( mInputFields.indexOf( mFocusedInput ) + 1 ) % n;
    for ( int i = 0; i < n && mInputFields[nextIdx]->isHidden(); ++i )
    {
      nextIdx = ( nextIdx + 1 ) % n;
    }
    setFocusedInputField( mInputFields[nextIdx] );
    ev->accept();
  }
  else if ( ev->key() == Qt::Key_Backtab || ev->key() == Qt::Key_Up )
  {
    int n = mInputFields.size();
    int nextIdx = ( n + mInputFields.indexOf( mFocusedInput ) - 1 ) % n;
    for ( int i = 0; i < n && mInputFields[nextIdx]->isHidden(); ++i )
    {
      nextIdx = ( nextIdx - 1 ) % n;
    }
    setFocusedInputField( mInputFields[nextIdx] );
    ev->accept();
  }
  else
  {
    QWidget::keyPressEvent(( ev ) );
  }
}

void KadasFloatingInputWidget::showEvent( QShowEvent */*event*/ )
{
  int n = mInputFields.size();
  if ( mInitiallyFocusedInput >= 0 && mInitiallyFocusedInput < n )
  {
    int idx = mInitiallyFocusedInput;
    for ( int i = 0; i < n && mInputFields[idx]->isHidden(); ++i )
    {
      idx = ( idx + 1 ) % n;
    }
    setFocusedInputField( mInputFields[idx] );
  }
}

void KadasFloatingInputWidget::hideEvent( QHideEvent */*event*/ )
{
  // Move focus back to the canvas to ensure it receives key events
  setFocusedInputField( nullptr );
  mCanvas->setFocus();
}
