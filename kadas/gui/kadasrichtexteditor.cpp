/***************************************************************************
    kadasrichtexteditor.cpp
    -----------------------
    copyright            : (C) 2021 by Sandro Mani
                           (C) 2016 The Qt Company Ltd.
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

// Based on qttools-everywhere-src-5.15.2/src/designer/src/lib/shared/richtexteditor.cpp

#include <QAction>
#include <QColorDialog>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDomDocument>
#include <QFileDialog>
#include <QFontDatabase>
#include <QGridLayout>
#include <QImageReader>
#include <QLabel>
#include <QLineEdit>
#include <QPainter>
#include <QPushButton>
#include <QSpinBox>
#include <QTextBlock>
#include <QToolBar>
#include <QToolButton>
#include <QUrlQuery>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include <qgis/qgsapplication.h>
#include <qgis/qgsproject.h>
#include <qgis/qgssettings.h>

#include <kadas/gui/kadasrichtexteditor.h>

// Richtext simplification filter helpers: Elements to be discarded
static inline bool filterElement( const QStringRef &name )
{
  return name != QStringLiteral( "meta" ) && name != QStringLiteral( "style" );
}

// Richtext simplification filter helpers: Filter attributes of elements
static inline void filterAttributes( const QStringRef &name,
                                     QXmlStreamAttributes *atts,
                                     bool *paragraphAlignmentFound )
{
  if ( atts->isEmpty() )
    return;

  // No style attributes for <body>
  if ( name == QStringLiteral( "body" ) )
  {
    atts->clear();
    return;
  }

  // Clean out everything except 'align' for 'p'
  if ( name == QStringLiteral( "p" ) )
  {
    for ( auto it = atts->begin(); it != atts->end(); )
    {
      if ( it->name() == QStringLiteral( "align" ) )
      {
        ++it;
        *paragraphAlignmentFound = true;
      }
      else
      {
        it = atts->erase( it );
      }
    }
    return;
  }
}

// Richtext simplification filter helpers: Check for blank QStringRef.
static inline bool isWhiteSpace( const QStringRef &in )
{
  const int count = in.size();
  for ( int i = 0; i < count; i++ )
    if ( !in.at( i ).isSpace() )
      return false;
  return true;
}

// Richtext simplification filter: Remove hard-coded font settings,
// <style> elements, <p> attributes other than 'align' and
// and unnecessary meta-information.
QString simplifyRichTextFilter( const QString &in, bool *isPlainTextPtr = nullptr )
{
  unsigned elementCount = 0;
  bool paragraphAlignmentFound = false;
  QString out;
  QXmlStreamReader reader( in );
  QXmlStreamWriter writer( &out );
  writer.setAutoFormatting( false );
  writer.setAutoFormattingIndent( 0 );

  while ( !reader.atEnd() )
  {
    switch ( reader.readNext() )
    {
      case QXmlStreamReader::StartElement:
        elementCount++;
        if ( filterElement( reader.name() ) )
        {
          const auto name = reader.name();
          QXmlStreamAttributes attributes = reader.attributes();
          filterAttributes( name, &attributes, &paragraphAlignmentFound );
          writer.writeStartElement( name.toString() );
          if ( !attributes.isEmpty() )
            writer.writeAttributes( attributes );
        }
        else
        {
          reader.readElementText(); // Skip away all nested elements and characters.
        }
        break;
      case QXmlStreamReader::Characters:
        if ( !isWhiteSpace( reader.text() ) )
          writer.writeCharacters( reader.text().toString() );
        break;
      case QXmlStreamReader::EndElement:
        writer.writeEndElement();
        break;
      default:
        break;
    }
  }
  // Check for plain text (no spans, just <html><head><body><p>)
  if ( isPlainTextPtr )
    *isPlainTextPtr = !paragraphAlignmentFound && elementCount == 4u; //
  return out;
}

static QAction *createCheckableAction( const QIcon &icon, const QString &text,
                                       QObject *receiver, const char *slot,
                                       QObject *parent = nullptr )
{
  QAction *result = new QAction( parent );
  result->setIcon( icon );
  result->setText( text );
  result->setCheckable( true );
  result->setChecked( false );
  if ( slot )
    QObject::connect( result, SIGNAL( triggered( bool ) ), receiver, slot );
  return result;
}

///////////////////////////////////////////////////////////////////////////////

KadasAddLinkDialog::KadasAddLinkDialog( KadasRichTextEditor *editor, QWidget *parent )
  : QDialog( parent )
  , m_editor( editor )
{
  setWindowTitle( tr( "Add Link" ) );

  m_titleInput = new QLineEdit();
  m_urlInput = new QLineEdit();

  QDialogButtonBox *bbox = new QDialogButtonBox( QDialogButtonBox::Ok | QDialogButtonBox::Cancel );
  connect( bbox, &QDialogButtonBox::accepted, this, &KadasAddLinkDialog::accept );
  connect( bbox, &QDialogButtonBox::rejected, this, &KadasAddLinkDialog::reject );

  QGridLayout *layout = new QGridLayout();
  layout->addWidget( new QLabel( tr( "Title:" ) ), 0, 0 );
  layout->addWidget( m_titleInput, 0, 1 );
  layout->addWidget( new QLabel( tr( "URL:" ) ), 1, 0 );
  layout->addWidget( m_urlInput, 1, 1 );
  layout->addWidget( bbox, 2, 0, 1, 2 );
  setLayout( layout );
}

int KadasAddLinkDialog::showDialog()
{
  // Set initial focus
  const QTextCursor cursor = m_editor->textCursor();
  if ( cursor.hasSelection() )
  {
    m_titleInput->setText( cursor.selectedText() );
    m_urlInput->setFocus();
  }
  else
  {
    m_titleInput->setFocus();
  }

  return exec();
}

void KadasAddLinkDialog::accept()
{
  const QString title = m_titleInput->text();
  const QString url = m_urlInput->text();

  if ( !title.isEmpty() )
  {
    m_editor->insertHtml( QStringLiteral( "<a href=\"%1\">%2</a>" ).arg( url ).arg( title ) );
  }

  m_titleInput->clear();
  m_urlInput->clear();

  QDialog::accept();
}

///////////////////////////////////////////////////////////////////////////////

KadasAddImageDialog::KadasAddImageDialog( KadasRichTextEditor *editor, QWidget *parent )
  : QDialog( parent )
  , m_editor( editor )
{
  setWindowTitle( tr( "Add Image" ) );

  m_urlInput = new QLineEdit();
  m_urlInput->setReadOnly( true );
  m_widthInput = new QSpinBox();
  m_widthInput->setRange( 16, 8192 );
  m_heightInput = new QSpinBox();
  m_heightInput->setRange( 16, 8192 );
  m_aspectButton = new QToolButton();
  m_aspectButton->setIcon( QIcon( ":/kadas/icons/vchain" ) );
  m_aspectButton->setCheckable( true );
  m_aspectButton->setAutoRaise( false );
  m_aspectButton->setChecked( true );
  QPushButton *browseButton = new QPushButton( QgsApplication::getThemeIcon( "/mActionFileOpen.svg" ), QString() );
  m_bbox = new QDialogButtonBox( QDialogButtonBox::Ok | QDialogButtonBox::Cancel );
  m_bbox->button( QDialogButtonBox::Ok )->setEnabled( false );

  connect( browseButton, &QPushButton::clicked, this, &KadasAddImageDialog::showFileDialog );
  connect( m_widthInput, qOverload<int>( &QSpinBox::valueChanged ), this, &KadasAddImageDialog::widthChanged );
  connect( m_heightInput, qOverload<int>( &QSpinBox::valueChanged ), this, &KadasAddImageDialog::heightChanged );
  connect( m_aspectButton, &QToolButton::toggled, this, &KadasAddImageDialog::widthChanged );
  connect( m_bbox, &QDialogButtonBox::accepted, this, &KadasAddImageDialog::accept );
  connect( m_bbox, &QDialogButtonBox::rejected, this, &KadasAddImageDialog::reject );

  QGridLayout *layout = new QGridLayout();
  layout->addWidget( new QLabel( tr( "URL:" ) ), 0, 0 );
  layout->addWidget( m_urlInput, 0, 1 );
  layout->addWidget( browseButton, 0, 2 );
  layout->addWidget( new QLabel( tr( "Width:" ) ), 1, 0 );
  layout->addWidget( m_widthInput, 1, 1 );
  layout->addWidget( new QLabel( tr( "Height:" ) ), 2, 0 );
  layout->addWidget( m_heightInput, 2, 1 );
  layout->addWidget( m_aspectButton, 1, 2, 2, 1 );
  layout->addWidget( m_bbox, 3, 0, 1, 3 );
  setLayout( layout );
}

void KadasAddImageDialog::showFileDialog()
{
  QString lastDir = QgsSettings().value( "/UI/lastImportExportDir", "." ).toString();
  QSet<QString> formats;
  for ( const QByteArray &format : QImageReader::supportedImageFormats() )
  {
    formats.insert( QString( "*.%1" ).arg( QString( format ).toLower() ) );
  }

  QString filter = QString( "Images (%1)" ).arg( QStringList( formats.values() ).join( " " ) );
  QString filename = QFileDialog::getOpenFileName( this, tr( "Select Image" ), lastDir, filter );
  if ( filename.isEmpty() )
  {
    m_bbox->button( QDialogButtonBox::Ok )->setEnabled( false );
    return;
  }
  QgsSettings().setValue( "/UI/lastImportExportDir", QFileInfo( filename ).absolutePath() );
  m_urlInput->setText( filename );
  m_imageSize = QImageReader( filename ).size();
  m_bbox->button( QDialogButtonBox::Ok )->setEnabled( true );
  m_widthInput->blockSignals( true );
  m_heightInput->blockSignals( true );
  m_widthInput->setValue( 192 );
  m_heightInput->setValue( 192. / m_imageSize.width() * m_imageSize.height() );
  m_widthInput->blockSignals( false );
  m_heightInput->blockSignals( false );
}

void KadasAddImageDialog::widthChanged()
{
  if ( m_aspectButton->isChecked() )
  {
    m_heightInput->blockSignals( true );
    m_heightInput->setValue( m_widthInput->value() / double( m_imageSize.width() ) * m_imageSize.height() );
    m_heightInput->blockSignals( false );
  }
}

void KadasAddImageDialog::heightChanged()
{
  if ( m_aspectButton->isChecked() )
  {
    m_widthInput->blockSignals( true );
    m_widthInput->setValue( m_heightInput->value() / double( m_imageSize.height() ) * m_imageSize.width() );
    m_widthInput->blockSignals( false );
  }
}

void KadasAddImageDialog::accept()
{
  QString attachedFilename = QgsProject::instance()->createAttachedFile( QFileInfo( m_urlInput->text() ).fileName() );
  QFile file( m_urlInput->text() );
  QFile attachedFile( attachedFilename );
  if ( file.open( QIODevice::ReadOnly ) && attachedFile.open( QIODevice::WriteOnly ) )
  {
    attachedFile.write( file.readAll() );
  }
  QString attachmentId = QgsProject::instance()->attachmentIdentifier( attachedFilename );
  QString url = QStringLiteral( "%1?w=%2&h=%3" ).arg( attachmentId ).arg( m_widthInput->value() ).arg( m_heightInput->value() );
  m_editor->insertHtml( QStringLiteral( "<img src=\"%1\" />" ).arg( url ) );
  QDialog::accept();
}

///////////////////////////////////////////////////////////////////////////////

KadasColorAction::KadasColorAction( QObject *parent ):
  QAction( parent )
{
  setText( tr( "Text Color" ) );
  setColor( Qt::black );
  connect( this, &QAction::triggered, this, &KadasColorAction::chooseColor );
}

void KadasColorAction::setColor( const QColor &color )
{
  if ( color == m_color )
    return;
  m_color = color;
  QPixmap pix( 24, 24 );
  QPainter painter( &pix );
  painter.setRenderHint( QPainter::Antialiasing, false );
  painter.fillRect( pix.rect(), m_color );
  painter.setPen( m_color.darker() );
  painter.drawRect( pix.rect().adjusted( 0, 0, -1, -1 ) );
  setIcon( pix );
}

void KadasColorAction::chooseColor()
{
  const QColor col = QColorDialog::getColor( m_color, nullptr );
  if ( col.isValid() && col != m_color )
  {
    setColor( col );
    emit colorChanged( m_color );
  }
}

///////////////////////////////////////////////////////////////////////////////

KadasRichTextEditorToolBar::KadasRichTextEditorToolBar( KadasRichTextEditor *editor, QWidget *parent ) :
  QToolBar( parent ),
  m_link_action( new QAction( this ) ),
  m_image_action( new QAction( this ) ),
  m_color_action( new KadasColorAction( this ) ),
  m_font_size_input( new QComboBox ),
  m_editor( editor )
{
  // Font size combo box
  m_font_size_input->setEditable( false );
  const auto font_sizes = QFontDatabase::standardSizes();
  for ( int font_size : font_sizes )
    m_font_size_input->addItem( QString::number( font_size ) );

  connect( m_font_size_input, &QComboBox::textActivated,
           this, &KadasRichTextEditorToolBar::sizeInputActivated );
  addWidget( m_font_size_input );

  // Text color button
  connect( m_color_action, &KadasColorAction::colorChanged,
           this, &KadasRichTextEditorToolBar::colorChanged );
  addAction( m_color_action );

  addSeparator();

  // Bold, italic and underline buttons

  m_bold_action = createCheckableAction( QIcon( ":/kadas/icons/texteditor/textbold" ), tr( "Bold" ), editor, SLOT( setFontBold( bool ) ), this );
  m_bold_action->setShortcut( QKeySequence( Qt::CTRL + Qt::Key_B ) );
  addAction( m_bold_action );

  m_italic_action = createCheckableAction( QIcon( ":/kadas/icons/texteditor/textitalic" ), tr( "Italic" ), editor, SLOT( setFontItalic( bool ) ), this );
  m_italic_action->setShortcut( QKeySequence( Qt::CTRL + Qt::Key_I ) );
  addAction( m_italic_action );

  m_underline_action = createCheckableAction( QIcon( ":/kadas/icons/texteditor/textunder" ), tr( "Underline" ), editor, SLOT( setFontUnderline( bool ) ), this );
  m_underline_action->setShortcut( QKeySequence( Qt::CTRL + Qt::Key_U ) );
  addAction( m_underline_action );

  addSeparator();

  // Left, center, right and justified alignment buttons

  QActionGroup *alignment_group = new QActionGroup( this );
  connect( alignment_group, &QActionGroup::triggered,
           this, &KadasRichTextEditorToolBar::alignmentActionTriggered );

  m_align_left_action = createCheckableAction( QIcon( ":/kadas/icons/texteditor/textleft" ), tr( "Left Align" ), editor, nullptr, alignment_group );
  addAction( m_align_left_action );

  m_align_center_action = createCheckableAction( QIcon( ":/kadas/icons/texteditor/textcenter" ), tr( "Center" ), editor, nullptr, alignment_group );
  addAction( m_align_center_action );

  m_align_right_action = createCheckableAction( QIcon( ":/kadas/icons/texteditor/textright" ), tr( "Right Align" ), editor, nullptr, alignment_group );
  addAction( m_align_right_action );

  m_align_justify_action = createCheckableAction( QIcon( ":/kadas/icons/texteditor/textjustify" ), tr( "Justify" ), editor, nullptr, alignment_group );
  addAction( m_align_justify_action );

  addSeparator();

  // Superscript and subscript buttons

  m_valign_sup_action = createCheckableAction( QIcon( ":/kadas/icons/texteditor/textsuperscript" ), tr( "Superscript" ), this, SLOT( setVAlignSuper( bool ) ), this );
  addAction( m_valign_sup_action );

  m_valign_sub_action = createCheckableAction( QIcon( ":/kadas/icons/texteditor/textsubscript" ), tr( "Subscript" ), this, SLOT( setVAlignSub( bool ) ), this );
  addAction( m_valign_sub_action );

  addSeparator();

  // Insert hyperlink and image buttons

  m_link_action->setIcon( QIcon( ":/kadas/icons/texteditor/textanchor" ) );
  m_link_action->setText( tr( "Insert &Link" ) );
  connect( m_link_action, &QAction::triggered, this, &KadasRichTextEditorToolBar::insertLink );
  addAction( m_link_action );

  m_image_action->setIcon( QIcon( ":/kadas/icons/texteditor/insertimage" ) );
  m_image_action->setText( tr( "Insert &Image" ) );
  connect( m_image_action, &QAction::triggered, this, &KadasRichTextEditorToolBar::insertImage );
  addAction( m_image_action );

  connect( editor, &QTextEdit::textChanged, this, &KadasRichTextEditorToolBar::updateActions );
  connect( editor, &KadasRichTextEditor::stateChanged, this, &KadasRichTextEditorToolBar::updateActions );

  updateActions();
}

void KadasRichTextEditorToolBar::alignmentActionTriggered( QAction *action )
{
  if ( action == m_align_left_action )
  {
    m_editor->setAlignment( Qt::AlignLeft );
  }
  else if ( action == m_align_center_action )
  {
    m_editor->setAlignment( Qt::AlignCenter );
  }
  else if ( action == m_align_right_action )
  {
    m_editor->setAlignment( Qt::AlignRight );
  }
  else
  {
    m_editor->setAlignment( Qt::AlignJustify );
  }
}

void KadasRichTextEditorToolBar::colorChanged( const QColor &color )
{
  m_editor->setTextColor( color );
  m_editor->setFocus();
}

void KadasRichTextEditorToolBar::sizeInputActivated( const QString &size )
{
  bool ok;
  int i = size.toInt( &ok );
  if ( !ok )
    return;

  m_editor->setFontPointSize( i );
  m_editor->setFocus();
}

void KadasRichTextEditorToolBar::setVAlignSuper( bool super )
{
  const QTextCharFormat::VerticalAlignment align = super ?
      QTextCharFormat::AlignSuperScript : QTextCharFormat::AlignNormal;

  QTextCharFormat charFormat = m_editor->currentCharFormat();
  charFormat.setVerticalAlignment( align );
  m_editor->setCurrentCharFormat( charFormat );

  m_valign_sub_action->setChecked( false );
}

void KadasRichTextEditorToolBar::setVAlignSub( bool sub )
{
  const QTextCharFormat::VerticalAlignment align = sub ?
      QTextCharFormat::AlignSubScript : QTextCharFormat::AlignNormal;

  QTextCharFormat charFormat = m_editor->currentCharFormat();
  charFormat.setVerticalAlignment( align );
  m_editor->setCurrentCharFormat( charFormat );

  m_valign_sup_action->setChecked( false );
}

void KadasRichTextEditorToolBar::insertLink()
{
  KadasAddLinkDialog linkDialog( m_editor, this );
  linkDialog.showDialog();
  m_editor->setFocus();
}

void KadasRichTextEditorToolBar::insertImage()
{
  KadasAddImageDialog( m_editor, this ).exec();
}

void KadasRichTextEditorToolBar::updateActions()
{
  if ( m_editor == nullptr )
  {
    setEnabled( false );
    return;
  }

  const Qt::Alignment alignment = m_editor->alignment();
  const QTextCursor cursor = m_editor->textCursor();
  const QTextCharFormat charFormat = cursor.charFormat();
  const QFont font = charFormat.font();
  const QTextCharFormat::VerticalAlignment valign =
    charFormat.verticalAlignment();
  const bool superScript = valign == QTextCharFormat::AlignSuperScript;
  const bool subScript = valign == QTextCharFormat::AlignSubScript;

  if ( alignment & Qt::AlignLeft )
  {
    m_align_left_action->setChecked( true );
  }
  else if ( alignment & Qt::AlignRight )
  {
    m_align_right_action->setChecked( true );
  }
  else if ( alignment & Qt::AlignHCenter )
  {
    m_align_center_action->setChecked( true );
  }
  else
  {
    m_align_justify_action->setChecked( true );
  }

  m_bold_action->setChecked( font.bold() );
  m_italic_action->setChecked( font.italic() );
  m_underline_action->setChecked( font.underline() );
  m_valign_sup_action->setChecked( superScript );
  m_valign_sub_action->setChecked( subScript );

  const int size = font.pointSize();
  const int idx = m_font_size_input->findText( QString::number( size ) );
  if ( idx != -1 )
    m_font_size_input->setCurrentIndex( idx );

  m_color_action->setColor( m_editor->textColor() );
}

///////////////////////////////////////////////////////////////////////////////

KadasRichTextEditor::KadasRichTextEditor( QWidget *parent )
  : QTextEdit( parent )
{
  connect( this, &KadasRichTextEditor::currentCharFormatChanged,
           this, &KadasRichTextEditor::stateChanged );
  connect( this, &KadasRichTextEditor::cursorPositionChanged,
           this, &KadasRichTextEditor::stateChanged );
  connect( this->document(), &QTextDocument::contentsChange, this, &KadasRichTextEditor::checkImageRemoved );
}

QVariant KadasRichTextEditor::loadResource( int type, const QUrl &url )
{
  if ( type == QTextDocument::ImageResource )
  {
    if ( url.scheme() == "attachment" )
    {
      QString path = url.path();
      int width = QUrlQuery( url.query() ).queryItemValue( "w" ).toInt();
      int height = QUrlQuery( url.query() ).queryItemValue( "h" ).toInt();
      QString attachmentId = QStringLiteral( "%1://%2" ).arg( url.scheme() ).arg( url.path() );
      QString attachmentFile = QgsProject::instance()->resolveAttachmentIdentifier( attachmentId );
      if ( !attachmentFile.isEmpty() )
      {
        return QImage( attachmentFile ).scaled( width, height, Qt::IgnoreAspectRatio, Qt::SmoothTransformation );
      }
    }
  }
  return QTextEdit::loadResource( type, url );
}

void KadasRichTextEditor::checkImageRemoved( int position, int charsRemoved, int /*charsAdded*/ )
{
  if ( charsRemoved > 0 )
  {
    document()->undo();
    QTextCursor c( textCursor() );
    for ( int i = position; i < position + charsRemoved; ++i )
    {
      c.setPosition( i );
      c.setPosition( i + 1, QTextCursor::KeepAnchor );
      QTextCharFormat fmt = c.charFormat();
      if ( fmt.isImageFormat() )
      {
        QUrl url( fmt.toImageFormat().name() );
        if ( url.scheme() == "attachment" )
        {
          QString attachmentId = QStringLiteral( "%1://%2" ).arg( url.scheme() ).arg( url.path() );
          QString attachedFile = QgsProject::instance()->resolveAttachmentIdentifier( attachmentId );
          if ( !attachedFile.isEmpty() )
          {
            QgsProject::instance()->removeAttachedFile( attachedFile );
          }
        }
      }
    }
    redo();
  }
}

void KadasRichTextEditor::setFontBold( bool b )
{
  setFontWeight( b ? QFont::Bold : QFont::Normal );
}

void KadasRichTextEditor::setFontPointSize( double d )
{
  QTextEdit::setFontPointSize( qreal( d ) );
}

void KadasRichTextEditor::setText( const QString &text )
{
  if ( Qt::mightBeRichText( text ) )
    setHtml( text );
  else
    setPlainText( text );
}

QString KadasRichTextEditor::text( Qt::TextFormat format ) const
{
  switch ( format )
  {
    case Qt::PlainText:
      return toPlainText();
    case Qt::RichText:
      return simplifyRichTextFilter( toHtml() );
    default:
      break;
  }
  const QString html = toHtml();
  bool isPlainText;
  const QString simplifiedHtml = simplifyRichTextFilter( html, &isPlainText );
  if ( isPlainText )
    return toPlainText();
  return simplifiedHtml;
}
