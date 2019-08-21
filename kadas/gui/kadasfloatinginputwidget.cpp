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
#include <kadas/gui/kadasfloatinginputwidget.h>

KadasFloatingInputWidgetField::KadasFloatingInputWidgetField( QValidator *validator, QWidget *parent )
  : QLineEdit( parent )
{
  setValidator( validator );
  connect( this, &KadasFloatingInputWidgetField::returnPressed, this, &KadasFloatingInputWidgetField::checkInputChanged );
}

KadasFloatingInputWidgetField::KadasFloatingInputWidgetField( int id, int decimals, double min, double max, QWidget *parent )
  : QLineEdit( parent )
  , mId( id )
  , mDecimals( decimals )
{
  QDoubleValidator *validator = new QDoubleValidator( min, max, decimals );
  setValidator( validator );
  connect( this, &KadasFloatingInputWidgetField::returnPressed, this, &KadasFloatingInputWidgetField::checkInputChanged );
}

void KadasFloatingInputWidgetField::setText( const QString &text )
{
  QLineEdit::setText( text );
  mPrevText = text;
}

void KadasFloatingInputWidgetField::setValue( double value )
{
  QLineEdit::setText( QString::number( value, 'f', mDecimals ) );
  mPrevText = text();
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
  QGridLayout *gridLayout = new QGridLayout();
  gridLayout->setContentsMargins( 2, 2, 2, 2 );
  gridLayout->setSpacing( 1 );
  gridLayout->setSizeConstraint( QLayout::SetFixedSize );
  setLayout( gridLayout );
  // Initially out of sight
  move( -1000, -1000 );
}

int KadasFloatingInputWidget::addInputField( const QString &label, KadasFloatingInputWidgetField *widget, bool initiallyfocused )
{
  QGridLayout *gridLayout = static_cast<QGridLayout *>( layout() );
  int row = gridLayout->isEmpty() ? 0 : gridLayout->rowCount();
  gridLayout->addWidget( new QLabel( label ), row, 0, 1, 1 );;
  gridLayout->addWidget( widget, row, 1, 1, 1 );
  mInputFields.insert( widget->id(), widget );
  if ( initiallyfocused )
  {
    setFocusedInputField( widget );
    mInitiallyFocusedInput = widget->id();
  }
  return row;
}

void KadasFloatingInputWidget::setInputFieldVisible( int idx, bool visible )
{
  QGridLayout *gridLayout = static_cast<QGridLayout *>( layout() );
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

void KadasFloatingInputWidget::setFocusedInputField( KadasFloatingInputWidgetField *widget )
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

void KadasFloatingInputWidget::ensureFocus()
{
  if ( !mFocusedInput && !mInputFields.isEmpty() )
  {
    setFocusedInputField( mInputFields[0] );
  }
}

void KadasFloatingInputWidget::adjustCursorAndExtent( const QgsPointXY &geoPos )
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
    auto nextIt = ( ++mInputFields.find( mFocusedInput->id() ) );
    if ( nextIt == mInputFields.end() )
    {
      nextIt = mInputFields.begin();
    }
    setFocusedInputField( mInputFields[nextIt.key()] );
    ev->accept();
  }
  else if ( ev->key() == Qt::Key_Backtab || ev->key() == Qt::Key_Up )
  {
    auto it = mInputFields.find( mFocusedInput->id() );
    if ( it == mInputFields.begin() )
    {
      it = --mInputFields.end();
    }
    else
    {
      --it;
    }
    setFocusedInputField( mInputFields[it.key()] );
    ev->accept();
  }
  else
  {
    QWidget::keyPressEvent( ( ev ) );
  }
}

void KadasFloatingInputWidget::showEvent( QShowEvent * /*event*/ )
{
  auto it = mInputFields.find( mInitiallyFocusedInput );
  if ( it != mInputFields.end() )
  {
    setFocusedInputField( it.value() );
  }
}

void KadasFloatingInputWidget::hideEvent( QHideEvent * /*event*/ )
{
  // Move focus back to the canvas to ensure it receives key events
  setFocusedInputField( nullptr );
  mCanvas->setFocus();
}
