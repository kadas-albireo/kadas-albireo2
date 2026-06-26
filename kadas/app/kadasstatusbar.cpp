/***************************************************************************
    kadasstatusbar.cpp
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

#include <QEvent>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QResizeEvent>
#include <QToolButton>

#include <qgis/qgsdoublespinbox.h>
#include <qgis/qgsscalecombobox.h>

#include "kadas/gui/kadascrsselection.h"

#include "kadasstatusbar.h"


KadasStatusBar::KadasStatusBar( QWidget *parent )
  : QWidget( parent )
{
  // Object name matches the application stylesheet (#mStatusWidget) so the
  // status bar and its children pick up the consistent dark styling.
  setObjectName( QStringLiteral( "mStatusWidget" ) );

  mLayout = new QHBoxLayout( this );
  mLayout->setContentsMargins( 0, 0, 0, 0 );
  mLayout->setSpacing( 7 );

  // Leading stretch keeps the content flush against the right edge of the bar
  // when there is more room than the content needs.
  mLayout->addStretch( 1 );

  // GPS ----------------------------------------------------------------
  mGpsLabel = new QLabel( tr( "GPS:" ), this );
  mGpsButton = new QToolButton( this );
  mGpsButton->setObjectName( QStringLiteral( "mGpsToolButton" ) );
  mGpsButton->setCheckable( true );
  mGpsButton->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::MinimumExpanding );
  // Fixed icon size so the GPS status indicator (a solid colour square painted
  // by KadasGpsIntegration) always renders, independent of the button's size.
  mGpsButton->setIconSize( QSize( 16, 16 ) );
  mGpsButton->setToolTip( tr( "<html><head/><body>Black: disconnected<br/>Blue: connecting<br/>White: no data<br/>Red: no fix<br/>Yellow: 2D fix<br/>Green: 3D fix</body></html>" ) );
  mLayout->addWidget( mGpsLabel );
  mLayout->addWidget( mGpsButton );

  // Mouse position -----------------------------------------------------
  mPositionLabel = new QLabel( tr( "Mouse position:" ), this );
  QFrame *coordinateFrame = new QFrame( this );
  coordinateFrame->setObjectName( QStringLiteral( "mCoordinateFrame" ) );
  coordinateFrame->setFrameShape( QFrame::StyledPanel );
  coordinateFrame->setFrameShadow( QFrame::Raised );
  QHBoxLayout *coordinateLayout = new QHBoxLayout( coordinateFrame );
  coordinateLayout->setContentsMargins( 0, 0, 0, 0 );
  coordinateLayout->setSpacing( 0 );
  mCoordinateEdit = new QLineEdit( coordinateFrame );
  mCoordinateEdit->setObjectName( QStringLiteral( "mCoordinateLineEdit" ) );
  mHeightEdit = new QLineEdit( coordinateFrame );
  mHeightEdit->setObjectName( QStringLiteral( "mHeightLineEdit" ) );
  coordinateLayout->addWidget( mCoordinateEdit );
  coordinateLayout->addWidget( mHeightEdit );
  mCoordinateEdit->setToolTip( tr( "Mouse position" ) );
  mHeightEdit->setToolTip( tr( "Height at mouse position" ) );

  mDisplayCrsButton = new QToolButton( this );
  mDisplayCrsButton->setObjectName( QStringLiteral( "mDisplayCRSButton" ) );
  mDisplayCrsButton->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::MinimumExpanding );
  mDisplayCrsButton->setPopupMode( QToolButton::InstantPopup );
  mDisplayCrsButton->setToolButtonStyle( Qt::ToolButtonTextOnly );
  mLayout->addWidget( mPositionLabel );
  mLayout->addWidget( coordinateFrame );
  mLayout->addWidget( mDisplayCrsButton );

  // Scale and magnifier ------------------------------------------------
  mScaleLabel = new QLabel( tr( "Scale:" ), this );
  mScaleCombo = new QgsScaleComboBox( this );
  mScaleCombo->setObjectName( QStringLiteral( "mScaleComboBox" ) );
  mScaleCombo->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::MinimumExpanding );
  mScaleCombo->setMinimumContentsLength( 14 );
  mScaleCombo->setToolTip( tr( "Map scale" ) );
  mScaleLockButton = new QToolButton( this );
  mScaleLockButton->setObjectName( QStringLiteral( "mScaleLockButton" ) );
  mScaleLockButton->setCheckable( true );
  mScaleLockButton->setIcon( QIcon( QStringLiteral( ":/kadas/icons/unlocked" ) ) );
  mScaleLockButton->setToolTip( tr( "Lock scale and zoom via magnification" ) );
  mMagnifierLabel = new QLabel( tr( "Magnifier:" ), this );
  mMagnifierSpinBox = new QgsDoubleSpinBox( this );
  mMagnifierSpinBox->setObjectName( QStringLiteral( "mMagnifierSpinBox" ) );
  mMagnifierSpinBox->setSuffix( QStringLiteral( "%" ) );
  mMagnifierSpinBox->setMaximum( 200.0 );
  mMagnifierSpinBox->setSingleStep( 50.0 );
  mMagnifierSpinBox->setValue( 100.0 );
  mLayout->addWidget( mScaleLabel );
  mLayout->addWidget( mScaleCombo );
  mLayout->addWidget( mScaleLockButton );
  mLayout->addWidget( mMagnifierLabel );
  mLayout->addWidget( mMagnifierSpinBox );

  // Coordinate system --------------------------------------------------
  mCrsLabel = new QLabel( tr( "Coordinate system:" ), this );
  mCrsSelectionButton = new KadasCrsSelection( this );
  mCrsSelectionButton->setObjectName( QStringLiteral( "mCRSSelectionButton" ) );
  mCrsSelectionButton->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::MinimumExpanding );
  mCrsSelectionButton->setToolTip( tr( "Coordinate system" ) );
  mLayout->addWidget( mCrsLabel );
  mLayout->addWidget( mCrsSelectionButton );

  cacheHintWidths();
}

QSize KadasStatusBar::sizeHint() const
{
  return QSize( mFullWidth, QWidget::sizeHint().height() );
}

QSize KadasStatusBar::minimumSizeHint() const
{
  return QSize( mCoreWidth, QWidget::minimumSizeHint().height() );
}

void KadasStatusBar::resizeEvent( QResizeEvent *event )
{
  QWidget::resizeEvent( event );
  updateResponsiveLayout();
}

void KadasStatusBar::changeEvent( QEvent *event )
{
  QWidget::changeEvent( event );
  if ( event->type() == QEvent::FontChange || event->type() == QEvent::StyleChange || event->type() == QEvent::LanguageChange )
  {
    cacheHintWidths();
    updateGeometry();
    updateResponsiveLayout();
  }
}

void KadasStatusBar::applyCollapseLevel( CollapseLevel level )
{
  // Lower-priority segments fold away first, each together with its own label,
  // so the descriptive labels of the segments that remain are kept as long as
  // there is room for them.
  const bool showMagnifier = level < HideMagnifier;
  mMagnifierLabel->setVisible( showMagnifier );
  mMagnifierSpinBox->setVisible( showMagnifier );

  const bool showScale = level < HideScale;
  mScaleLabel->setVisible( showScale );
  mScaleCombo->setVisible( showScale );
  mScaleLockButton->setVisible( showScale );

  const bool showGps = level < HideGps;
  mGpsLabel->setVisible( showGps );
  mGpsButton->setVisible( showGps );

  // Only when space is very tight do the remaining labels drop, leaving the
  // mouse coordinates and coordinate system as the core readout (tool tips kept).
  const bool showCoreLabels = level < HideLabels;
  mPositionLabel->setVisible( showCoreLabels );
  mCrsLabel->setVisible( showCoreLabels );
}

int KadasStatusBar::currentContentWidth() const
{
  mLayout->invalidate();
  return mLayout->sizeHint().width();
}

void KadasStatusBar::updateResponsiveLayout()
{
  if ( mUpdatingLayout )
    return;
  mUpdatingLayout = true;

  // Hysteresis band: a richer level is only restored once it fits with this
  // much room to spare, while collapsing happens as soon as the content
  // overflows. The dead zone between the two thresholds stops the layout from
  // toggling back and forth (which manifests as flickering) when the window is
  // dragged across a collapse boundary.
  const int hysteresis = 2 * mLayout->spacing() + fontMetrics().averageCharWidth() * 4;

  const int available = width();
  CollapseLevel level = mCurrentLevel;

  // Collapse further while the current level overflows the available width.
  applyCollapseLevel( level );
  while ( level < CollapseLevelCount - 1 && currentContentWidth() > available )
  {
    level = static_cast<CollapseLevel>( level + 1 );
    applyCollapseLevel( level );
  }

  // Expand again only when the next-richer level fits with the hysteresis
  // margin to spare, keeping a stable dead zone around each threshold.
  while ( level > ShowAll )
  {
    const CollapseLevel richer = static_cast<CollapseLevel>( level - 1 );
    applyCollapseLevel( richer );
    if ( currentContentWidth() <= available - hysteresis )
    {
      level = richer;
    }
    else
    {
      // The richer level would not clear the margin: revert and stop.
      applyCollapseLevel( level );
      break;
    }
  }

  mCurrentLevel = level;
  mUpdatingLayout = false;
}

void KadasStatusBar::cacheHintWidths()
{
  applyCollapseLevel( ShowAll );
  mFullWidth = currentContentWidth();

  applyCollapseLevel( HideLabels );
  mCoreWidth = currentContentWidth();

  // Restore to the full layout; the next resize event picks the right level.
  applyCollapseLevel( ShowAll );
}
