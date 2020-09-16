/***************************************************************************
    kadasribbonbutton.cpp
    ---------------------
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

#include <QBitmap>
#include <QDragEnterEvent>
#include <QPainter>
#include <QPainterPath>

#include <kadas/gui/kadasribbonbutton.h>


void KadasRibbonButton::paintEvent( QPaintEvent * /*e*/ )
{
  QPainter p( this );
  p.setRenderHint( QPainter::Antialiasing );

  // Background
  p.setBrush( QBrush( palette().color( QPalette::Window ) ) );
  if ( isChecked() )
  {
    p.setBrush( QBrush( QColor( 188, 210, 227 ) ) );
    p.setPen( QColor( 38, 59, 78 ) );
  }
  else if ( underMouse() && isEnabled() )
  {
    QPen underMousePen( palette().color( QPalette::Highlight ) );
    underMousePen.setWidth( 2 );
    p.setPen( underMousePen );
  }
  else if ( hasFocus() )
  {
    p.setPen( palette().color( QPalette::Highlight ) );
  }
  else
  {
    p.setPen( QColor( 255, 255, 255, 0 ) );
  }
  p.drawRoundedRect( 0, 0, width(), height(), 5, 5 );

  int iconBottomY = 0;
  bool smallIcon = false;
  if ( iconSize().height() <= 20 )
  {
    smallIcon = true;
  }

  if ( smallIcon )
  {
    iconBottomY = height() / 2.0 - 5.0;
  }
  else
  {
    iconBottomY = height() / 2.0 + 5;
  }

  // Menu arrow
  int buttonWidth = width();
  if ( menu() )
  {
    int y = height() - 6;
    p.setPen( Qt::transparent );
    p.setBrush( QColor( 38, 59, 78 ) );
    QPainterPath arrow;
    arrow.moveTo( width() - 12, y - 2 );
    arrow.lineTo( width() - 3, y - 2 );
    arrow.lineTo( width() - 7.5, y + 3 );
    arrow.closeSubpath();
    p.drawPath( arrow );
  }

  // Icon
  QIcon buttonIcon = icon();
  if ( !buttonIcon.isNull() )
  {
    QSize iSize = iconSize();
    int pixmapY = iconBottomY - iSize.height();
    int pixmapX = buttonWidth / 2.0 - iSize.width() / 2.0;
    if ( isEnabled() && !isChecked() )
    {
      QPixmap pixmap = buttonIcon.pixmap( iSize, QIcon::Normal, QIcon::On );
      p.drawPixmap( pixmapX, pixmapY, pixmap );
    }
    else
    {
      // Largest pixmap, to avoid issues with masking a downscaled image
      QPixmap pixmap = buttonIcon.pixmap( QSize( 1024, 1024 ), QIcon::Normal, QIcon::On );
      QBitmap mask = pixmap.createMaskFromColor( QColor( 0, 0, 0, 0 ), Qt::MaskInColor );
      p.save();
      p.setRenderHint( QPainter::SmoothPixmapTransform );
      p.setPen( QColor( 38, 59, 78 ) );
      p.drawPixmap( QRect( pixmapX, pixmapY, iSize.width(), iSize.height() ), mask );
      p.restore();
    }
  }

  // Text
  QString buttonText = text();
  if ( !buttonText.isEmpty() )
  {
    QFontMetricsF fm( font() );
    p.setFont( font() );
    if ( isChecked() )
    {
      p.setPen( QColor( 38, 59, 78 ) );
    }
    else if ( isEnabled() )
    {
      p.setPen( QColor( 255, 255, 255 ) );
    }
    else
    {
      p.setPen( QColor( 38, 59, 78 ) );
    }

    QStringList rawRextLines = buttonText.split( "\n" );
    // Insert additional line breaks where exceeds button width
    QStringList textLines;
    int maxWidth = buttonWidth - 10;
    for ( const QString &line : rawRextLines )
    {
      if ( fm.width( line ) > maxWidth )
      {
        QString newline;
        // Measure widths at whitespace pos, fit as many words into a line as possible
        for ( const QString &word : line.split( " " ) )
        {
          if ( fm.width( newline + " " + word ) > maxWidth )
          {
            if ( !newline.isEmpty() )
            {
              textLines.append( newline );
            }
            newline = word;
          }
          else
          {
            newline += QString( " " ) + word;
          }
        }
        textLines.append( newline );
      }
      else
      {
        textLines.append( line );
      }
    }

    for ( int i = 0; i < textLines.size(); ++i )
    {
      QString textLine = textLines.at( i );
      double textWidth = fm.width( textLine );
      double textHeight = fm.boundingRect( textLine ).height();
      int textX = ( buttonWidth - textWidth ) / 2.0;
      int textY = iconBottomY + textHeight * ( i + 1 ) /*+ i * 1*/;
      if ( smallIcon )
      {
        textY += 10;
      }
      p.drawText( textX, textY, textLine );
    }
  }
}

void KadasRibbonButton::enterEvent( QEvent *event )
{
  update();
  QToolButton::enterEvent( event );
}

void KadasRibbonButton::leaveEvent( QEvent *event )
{
  update();
  QToolButton::leaveEvent( event );
}

void KadasRibbonButton::focusInEvent( QFocusEvent *event )
{
  update();
  QToolButton::focusInEvent( event );
}

void KadasRibbonButton::focusOutEvent( QFocusEvent *event )
{
  update();
  QToolButton::focusOutEvent( event );
}

void KadasRibbonButton::mouseMoveEvent( QMouseEvent *event )
{
  event->ignore();
}

void KadasRibbonButton::mousePressEvent( QMouseEvent *event )
{
  QToolButton::mousePressEvent( event );
  event->ignore();
}

void KadasRibbonButton::contextMenuEvent( QContextMenuEvent *event )
{
  emit contextMenuRequested( event->pos() );
  event->accept();
}
