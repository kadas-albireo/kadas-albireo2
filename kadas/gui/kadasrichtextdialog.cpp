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
#include <QInputDialog>
#include <QMenu>
#include <QMessageBox>
#include <QLayout>
#include <QList>
#include <QRegularExpression>
#include <QFileInfo>
#include <QImage>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextFragment>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidgetAction>
#include <algorithm>

#include <QNetworkRequest>

#include <qgis/qgsnetworkaccessmanager.h>
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
  QToolBar *toolbar = mEditor->findChild<QToolBar *>();
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
      flow->addWidget( widget );
      widget->show();
      continue;
    }
    if ( action->objectName() == QLatin1String( "mActionInsertImage" ) )
    {
      // QgsRichTextEditor only offers a file chooser. Fold that and a URL
      // download into one menu, rather than spending a second slot of toolbar
      // width on it.
      auto *imageButton = new QToolButton();
      imageButton->setIcon( action->icon() );
      imageButton->setToolTip( action->toolTip() );
      imageButton->setAutoRaise( true );
      imageButton->setIconSize( QSize( iconSize, iconSize ) );
      imageButton->setPopupMode( QToolButton::InstantPopup );
      auto *menu = new QMenu( imageButton );
      menu->addAction( tr( "From File…" ), action, &QAction::trigger );
      menu->addAction( tr( "From URL…" ), this, &KadasRichTextDialog::insertImageFromUrl );
      imageButton->setMenu( menu );
      flow->addWidget( imageButton );
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
  connect( mEditor, &QgsRichTextEditor::textChanged, this, [this] {
    if ( mClampingImages )
      return;
    mClampingImages = true;
    KadasAttachmentUtils::clampImageDisplaySize( mEditor->document() );
    mClampingImages = false;
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
  static const QRegularExpression sUrlRe( QStringLiteral( R"((?:https?://|www\.)[^\s<>"']+)" ), QRegularExpression::CaseInsensitiveOption );

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
          url.chop( 1 );
        if ( url.isEmpty() )
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

void KadasRichTextDialog::insertImageFromUrl()
{
  bool ok = false;
  const QString entered = QInputDialog::getText( this, tr( "Image from URL" ), tr( "Enter the URL of an image:" ), QLineEdit::Normal, QString(), &ok );
  if ( !ok || entered.trimmed().isEmpty() )
    return;

  const QUrl url( entered.trimmed() );
  if ( !url.isValid() || !( url.scheme().compare( QLatin1String( "http" ), Qt::CaseInsensitive ) == 0 || url.scheme().compare( QLatin1String( "https" ), Qt::CaseInsensitive ) == 0 ) )
  {
    QMessageBox::warning( this, tr( "Image from URL" ), tr( "Please enter a valid http:// or https:// URL." ) );
    return;
  }

  QNetworkRequest request( url );
  const QgsNetworkReplyContent content = QgsNetworkAccessManager::instance()->blockingGet( request );
  if ( content.error() != QNetworkReply::NoError || content.content().isEmpty() )
  {
    QMessageBox::warning( this, tr( "Image from URL" ), tr( "Failed to download the image: %1" ).arg( content.errorString() ) );
    return;
  }

  QImage image;
  if ( !image.loadFromData( content.content() ) )
  {
    QMessageBox::warning( this, tr( "Image from URL" ), tr( "That URL did not return an image." ) );
    return;
  }

  // Copied into the project rather than left as a remote reference, so the
  // description keeps working offline and once the URL rots.
  const QString identifier = KadasAttachmentUtils::attachImage( image, QFileInfo( url.path() ).suffix().toLower() );
  if ( identifier.isEmpty() )
  {
    QMessageBox::warning( this, tr( "Image from URL" ), tr( "Failed to store the image in the project." ) );
    return;
  }

  QTextImageFormat format;
  format.setName( identifier );
  format.setWidth( image.width() );
  format.setHeight( image.height() );
  // Inserted at its own size; the textChanged handler brings it down to the
  // size it will be shown at, exactly as for a file-chosen image.
  mEditor->textCursor().insertImage( format );
}

void KadasRichTextDialog::accept()
{
  // Only styling changes, so no position shifts to work around.
  linkifyBareUrls( mEditor->document() );
  mHtml = documentBody( KadasAttachmentUtils::materializeInlineImages( mEditor->toHtml() ) );
  // An emptied field still round-trips into a skeleton of markup; store nothing
  // at all, so callers can test it for emptiness.
  if ( mEditor->toPlainText().trimmed().isEmpty() && !mHtml.contains( QLatin1String( "<img" ), Qt::CaseInsensitive ) )
    mHtml.clear();
  QDialog::accept();
}
