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
#include <QStyle>
#include <QStyleOptionToolButton>

#include "kadas/gui/kadasribbonbutton.h"


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
    if ( popupMode() == QToolButton::MenuButtonPopup )
    {
      // Office-style split button: a vertical divider separates the main
      // (tool) area from the menu (arrow) area. The divider is drawn at the
      // left edge of the sub-control rectangle Qt uses to hit-test menu
      // clicks, so the visual split matches the clickable split exactly.
      QStyleOptionToolButton opt;
      initStyleOption( &opt );
      QRect menuRect = style()->subControlRect( QStyle::CC_ToolButton, &opt, QStyle::SC_ToolButtonMenu, this );
      if ( !menuRect.isValid() || menuRect.width() <= 0 )
      {
        menuRect = QRect( width() - 18, 0, 18, height() );
      }

      const int dividerX = menuRect.left();
      const int dividerMargin = 6;
      // Use the dark-blue ribbon background color so the split reads as the
      // ribbon showing through between the tool and the menu halves.
      QPen dividerPen( QColor( 0x26, 0x3B, 0x4E ) );
      dividerPen.setWidth( 1 );
      p.setPen( dividerPen );
      p.setBrush( Qt::NoBrush );
      p.drawLine( dividerX, dividerMargin, dividerX, height() - dividerMargin );

      const double arrowCx = menuRect.center().x() + 0.5;
      const double arrowCy = height() / 2.0;
      p.setPen( Qt::transparent );
      p.setBrush( QColor( 38, 59, 78 ) );
      QPainterPath arrow;
      arrow.moveTo( arrowCx - 4, arrowCy - 2 );
      arrow.lineTo( arrowCx + 4, arrowCy - 2 );
      arrow.lineTo( arrowCx, arrowCy + 3 );
      arrow.closeSubpath();
      p.drawPath( arrow );
    }
    else
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
  }

  // Icon
  QIcon buttonIcon = icon();
  if ( !buttonIcon.isNull() )
  {
    QSize iSize = iconSize();
    int pixmapY = iconBottomY - iSize.height();
    int pixmapX = buttonWidth / 2.0 - iSize.width() / 2.0;
    // Monochrome icon: white while idle, brand yellow when active (checked),
    // muted dark-blue when disabled. Keeps ribbon icons coherent and legible.
    QPixmap pixmap = buttonIcon.pixmap( QSize( 1024, 1024 ), QIcon::Normal, QIcon::On );
    const QRect iconRect( pixmapX, pixmapY, iSize.width(), iSize.height() );
    QColor iconColor = QColor( 38, 59, 78 );
    if ( isEnabled() )
      iconColor = isChecked() ? QColor( 0xFF, 0xCC, 0x00 ) : QColor( 255, 255, 255 );

    // Tint the icon through its alpha channel so edges keep their antialiasing
    // (a 1-bit mask would look jagged). The outline and fill share the exact
    // same silhouette, so the fill can never peek outside the outline.
    // Work in raw device pixels (force a 1.0 device-pixel ratio): on a
    // high-DPI screen the source pixmap carries a >1 ratio, and drawing it
    // into a ratio-1 buffer would otherwise paint only its top-left quarter,
    // making the icon render at half size.
    auto tintedPixmap = []( const QPixmap &source, const QColor &color ) {
      QPixmap src = source;
      src.setDevicePixelRatio( 1.0 );
      QPixmap out( src.size() );
      out.fill( Qt::transparent );
      QPainter pp( &out );
      pp.setRenderHint( QPainter::SmoothPixmapTransform );
      pp.drawPixmap( 0, 0, src );
      pp.setCompositionMode( QPainter::CompositionMode_SourceIn );
      pp.fillRect( out.rect(), color );
      pp.end();
      return out;
    };

    p.save();
    p.setRenderHint( QPainter::SmoothPixmapTransform );
    if ( isEnabled() && isChecked() )
    {
      // Black outline behind the yellow icon for contrast on the light fill.
      const QPixmap outline = tintedPixmap( pixmap, QColor( 0, 0, 0 ) );
      for ( int dx = -1; dx <= 1; ++dx )
        for ( int dy = -1; dy <= 1; ++dy )
          if ( dx != 0 || dy != 0 )
            p.drawPixmap( iconRect.translated( dx, dy ), outline );
    }
    p.drawPixmap( iconRect, tintedPixmap( pixmap, iconColor ) );
    p.restore();
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
      if ( fm.horizontalAdvance( line ) > maxWidth )
      {
        QString newline;
        // Measure widths at whitespace pos, fit as many words into a line as possible
        for ( const QString &word : line.split( " " ) )
        {
          if ( fm.horizontalAdvance( newline + " " + word ) > maxWidth )
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
      double textWidth = fm.horizontalAdvance( textLine );
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

void KadasRibbonButton::enterEvent( QEnterEvent *event )
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
