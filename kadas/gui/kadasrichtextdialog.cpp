/***************************************************************************
    kadasrichtextdialog.cpp
    -----------------------
    copyright            : (C) 2026 by Denis Rouzaud
    email                : denis at opengis dot ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <QBoxLayout>
#include <QDialogButtonBox>
#include <QApplication>
#include <QFrame>
#include <QLayout>
#include <QList>
#include <QRegularExpression>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextFragment>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidgetAction>
#include <algorithm>

#include <qgis/qgsrichtexteditor.h>

#include "kadas/gui/kadasattachmentutils.h"
#include "kadas/gui/kadasrichtextdialog.h"


//! Lays its items out left to right, starting a new row whenever the next one would not fit.
class KadasRichTextDialog::FlowLayout : public QLayout
{
  public:
    explicit FlowLayout( int spacing )
    {
      setContentsMargins( 0, 0, 0, 0 );
      setSpacing( spacing );
    }
    ~FlowLayout() override
    {
      while ( QLayoutItem *item = takeAt( 0 ) )
        delete item;
    }

    void addItem( QLayoutItem *item ) override { mItems.append( item ); }
    int count() const override { return mItems.size(); }
    QLayoutItem *itemAt( int index ) const override { return mItems.value( index ); }
    QLayoutItem *takeAt( int index ) override { return index >= 0 && index < mItems.size() ? mItems.takeAt( index ) : nullptr; }

    Qt::Orientations expandingDirections() const override { return {}; }
    bool hasHeightForWidth() const override { return true; }
    int heightForWidth( int width ) const override { return layoutItems( QRect( 0, 0, width, 0 ), false ); }
    void setGeometry( const QRect &rect ) override
    {
      QLayout::setGeometry( rect );
      layoutItems( rect, true );
    }

    QSize sizeHint() const override { return minimumSize(); }
    QSize minimumSize() const override
    {
      // One item wide: the layout can always fall back to a column, so it never
      // forces its container to be wider than the widest single button.
      QSize size;
      for ( const QLayoutItem *item : mItems )
        size = size.expandedTo( item->minimumSize() );
      return size;
    }

  private:
    int layoutItems( const QRect &rect, bool apply ) const
    {
      int x = rect.x();
      int y = rect.y();
      int rowHeight = 0;
      for ( QLayoutItem *item : mItems )
      {
        const QSize hint = item->sizeHint();
        if ( rowHeight > 0 && x + hint.width() > rect.right() + 1 )
        {
          x = rect.x();
          y += rowHeight + spacing();
          rowHeight = 0;
        }
        if ( apply )
          item->setGeometry( QRect( QPoint( x, y ), hint ) );
        x += hint.width() + spacing();
        rowHeight = std::max( rowHeight, hint.height() );
      }
      return y + rowHeight - rect.y();
    }

    QList<QLayoutItem *> mItems;
};

/**
 * Keeps only what is inside <body>. QTextDocument::toHtml() emits a whole
 * document, whose skeleton is bulk in the project file and whose body styling
 * would fight the styling of any document this markup is later embedded in.
 * Block-level formatting survives, since Qt writes it inline on each element.
 */
QString KadasRichTextDialog::documentBody( const QString &html )
{
  static const QRegularExpression sBodyRe( QStringLiteral( "<body[^>]*>(.*)</body>" ), QRegularExpression::DotMatchesEverythingOption | QRegularExpression::CaseInsensitiveOption );
  const QRegularExpressionMatch match = sBodyRe.match( html );
  return match.hasMatch() ? match.captured( 1 ).trimmed() : html;
}

/**
 * Rehosts \a editor's toolbar in a flow layout with smaller icons.
 *
 * A QToolBar cannot wrap: run out of width and it hides the overflow behind an
 * extension button, which for a formatting bar means the tools simply vanish.
 * Moving its contents into a flow layout gives a second row instead. The pass
 * also drops the tools that do not belong in this dialog.
 */
void KadasRichTextDialog::reflowToolbar( int iconSize )
{
  QToolBar *toolbar = mEditor->toolBar();
  auto *editorLayout = qobject_cast<QBoxLayout *>( mEditor->layout() );
  if ( !toolbar || !editorLayout )
    return;

  auto *container = new QWidget( mEditor );
  auto *flow = new FlowLayout( 2 );
  container->setLayout( flow );

  const QList<QAction *> actions = toolbar->actions();
  for ( QAction *action : actions )
  {
    // The HTML source view exposes how the description happens to be stored —
    // attachment:/// references, Qt's own markup — and invites writing markup
    // that Qt will then silently not render. The formatting tools are the
    // whole interface.
    if ( action->objectName() == QLatin1String( "mActionEditSource" ) )
      continue;
    if ( action->isSeparator() )
    {
      auto *line = new QFrame();
      line->setFrameShape( QFrame::VLine );
      line->setFrameShadow( QFrame::Sunken );
      flow->addWidget( line );
      continue;
    }
    if ( auto *widgetAction = qobject_cast<QWidgetAction *>( action ) )
    {
      QWidget *widget = widgetAction->defaultWidget();
      if ( !widget )
        continue;
      // The toolbar's expanding spacer exists only to right-align what follows
      // it; in a flow layout it would swallow a whole row.
      if ( widget->sizePolicy().horizontalPolicy() == QSizePolicy::Expanding )
        continue;
      toolbar->removeAction( action ); // releases the widget: unparented and hidden
      // The image button arrives as one of these, and would otherwise keep the
      // toolbar's own icon size while everything around it is reflowed.
      if ( auto *toolButton = qobject_cast<QToolButton *>( widget ) )
        toolButton->setIconSize( QSize( iconSize, iconSize ) );
      flow->addWidget( widget );
      widget->show();
      continue;
    }
    auto *button = new QToolButton();
    button->setDefaultAction( action );
    button->setAutoRaise( true );
    button->setIconSize( QSize( iconSize, iconSize ) );
    flow->addWidget( button );
  }

  editorLayout->replaceWidget( toolbar, container );
  toolbar->hide();
}


KadasRichTextDialog::KadasRichTextDialog( const QString &title, const QString &html, QWidget *parent )
  : QDialog( parent )
{
  setWindowTitle( title );
  setSizeGripEnabled( true );
  resize( 680, 520 );

  mEditor = new QgsRichTextEditor();
  // The markup is rendered by QTextDocument (the map tooltip, the form preview),
  // so offer the formatting that Qt's own HTML subset supports.
  mEditor->setMode( QgsRichTextEditor::Mode::QTextDocument );
  KadasAttachmentUtils::installResourceProvider( mEditor->document() );
  reflowToolbar( 16 );
  mEditor->setText( html );

  // QgsRichTextEditor inserts an image at its full pixel size, which in a photo's
  // case dwarfs the dialog. Bring it down to the size it will be shown at, as
  // soon as it lands, so the editor shows what the tooltip will.
  //
  // Driven by contentsChange rather than textChanged, so that typing - which can
  // never need a clamp - does not walk the whole document on every keystroke.
  connect( mEditor->document(), &QTextDocument::contentsChange, this, [this]( int position, int, int charsAdded ) {
    if ( mClampingImages || charsAdded == 0 )
      return;
    // An image is a single object replacement character; if the inserted range
    // holds none, there is nothing to size.
    QTextCursor inserted( mEditor->document() );
    inserted.setPosition( position );
    inserted.setPosition( position + charsAdded, QTextCursor::KeepAnchor );
    if ( !inserted.selectedText().contains( QChar::ObjectReplacementCharacter ) )
      return;
    // The document must not be edited from inside contentsChange, so do the
    // clamp once this change has finished being applied.
    mClampingImages = true;
    QMetaObject::invokeMethod(
      this,
      [this] {
        KadasAttachmentUtils::clampImageDisplaySize( mEditor->document() );
        mClampingImages = false;
      },
      Qt::QueuedConnection
    );
  } );

  auto *buttons = new QDialogButtonBox( QDialogButtonBox::Ok | QDialogButtonBox::Cancel );
  connect( buttons, &QDialogButtonBox::accepted, this, &KadasRichTextDialog::accept );
  connect( buttons, &QDialogButtonBox::rejected, this, &QDialog::reject );

  auto *layout = new QVBoxLayout( this );
  layout->addWidget( mEditor );
  layout->addWidget( buttons );
}

void KadasRichTextDialog::linkifyBareUrls( QTextDocument *document )
{
  static const QString sUrlPattern( QStringLiteral( R"((?:https?://|www\.)[^\s<>"']+)" ) );
  static const QRegularExpression sUrlRe( sUrlPattern, QRegularExpression::CaseInsensitiveOption );
  // Trimming the trailing punctuation can eat into the match's own prefix
  // ("www.)" leaves "www"), which is no longer a URL to link to.
  static const QRegularExpression sWholeUrlRe( QRegularExpression::anchoredPattern( sUrlPattern ), QRegularExpression::CaseInsensitiveOption );

  struct Link
  {
      int position = 0;
      int length = 0;
      QString href;
  };
  QList<Link> links;

  for ( QTextBlock block = document->begin(); block.isValid(); block = block.next() )
  {
    for ( QTextBlock::iterator it = block.begin(); !it.atEnd(); ++it )
    {
      const QTextFragment fragment = it.fragment();
      // Whatever the author linked deliberately stays as they left it.
      if ( !fragment.isValid() || fragment.charFormat().isAnchor() || fragment.charFormat().isImageFormat() )
        continue;
      const QString text = fragment.text();
      QRegularExpressionMatchIterator matches = sUrlRe.globalMatch( text );
      while ( matches.hasNext() )
      {
        const QRegularExpressionMatch match = matches.next();
        QString url = match.captured();
        // Punctuation hard against the end of a URL is far more often the
        // sentence's than the URL's, so leave it outside the link.
        while ( !url.isEmpty() && QStringLiteral( ".,;:!?)]}" ).contains( url.back() ) )
        {
          // Unless the URL opened it itself, as a Wikipedia title does:
          // ".../Foo_(bar)" ends in a bracket that is part of the address.
          const QChar last = url.back();
          if (
            ( last == QLatin1Char( ')' ) && url.count( QLatin1Char( '(' ) ) >= url.count( QLatin1Char( ')' ) ) )
            || ( last == QLatin1Char( ']' ) && url.count( QLatin1Char( '[' ) ) >= url.count( QLatin1Char( ']' ) ) )
            || ( last == QLatin1Char( '}' ) && url.count( QLatin1Char( '{' ) ) >= url.count( QLatin1Char( '}' ) ) )
          )
            break;
          url.chop( 1 );
        }
        if ( !sWholeUrlRe.match( url ).hasMatch() )
          continue;
        const QString href = url.startsWith( QLatin1String( "www." ), Qt::CaseInsensitive ) ? QStringLiteral( "http://" ) + url : url;
        links.append( { static_cast<int>( fragment.position() + match.capturedStart() ), static_cast<int>( url.length() ), href } );
      }
    }
  }

  if ( links.isEmpty() )
    return;

  QTextCursor cursor( document );
  cursor.beginEditBlock();
  for ( const Link &link : std::as_const( links ) )
  {
    cursor.setPosition( link.position );
    cursor.setPosition( link.position + link.length, QTextCursor::KeepAnchor );
    QTextCharFormat format = cursor.charFormat();
    format.setAnchor( true );
    format.setAnchorHref( link.href );
    format.setForeground( QApplication::palette().link() );
    format.setFontUnderline( true );
    cursor.setCharFormat( format );
  }
  cursor.endEditBlock();
}

void KadasRichTextDialog::accept()
{
  // Only styling changes, so no position shifts to work around.
  linkifyBareUrls( mEditor->document() );
  mHtml = documentBody( KadasAttachmentUtils::materializeInlineImages( mEditor->toHtml() ) );

  // Qt spells the document's default styling out on every paragraph it writes,
  // so a line nobody formatted still round-trips into a screenful of markup —
  // stored, and carried by every copy of the project from then on. Rebuilding
  // the document from its own plain text says whether that markup holds
  // anything the plain text does not: when the two agree, it does not, and the
  // plain text is what gets stored. Proving them equal beats guessing at which
  // properties count as formatting, which risks dropping some that do.
  const QString plainText = mEditor->document()->toPlainText();
  QTextDocument plain;
  plain.setDefaultFont( mEditor->document()->defaultFont() );
  plain.setPlainText( plainText );
  if ( documentBody( plain.toHtml() ) == mHtml )
    mHtml = plainText;

  // An emptied field still round-trips into a skeleton of markup; store nothing
  // at all, so callers can test it for emptiness.
  if ( plainText.trimmed().isEmpty() && !mHtml.contains( QLatin1String( "<img" ), Qt::CaseInsensitive ) )
    mHtml.clear();
  QDialog::accept();
}
