/***************************************************************************
    kadasstatusbar.h
    ----------------
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

#ifndef KADASSTATUSBAR_H
#define KADASSTATUSBAR_H

#include <QWidget>

class QHBoxLayout;
class QLabel;
class QLineEdit;
class QToolButton;
class QgsDoubleSpinBox;
class QgsScaleComboBox;
class KadasCrsSelection;

/**
 * The permanent widget shown on the right of the main window status bar.
 *
 * It hosts the GPS indicator, the mouse coordinate / height readout, the map
 * scale and magnifier controls and the coordinate-system selector.
 *
 * The widget is responsive: as the available width shrinks it progressively
 * collapses, folding away the lower-priority segments first (the magnifier,
 * then the scale controls, then the GPS indicator) so the descriptive text
 * labels are kept as long as there is room for them. Only when space is very
 * tight are the remaining labels dropped (the value widgets keep tool tips),
 * leaving the mouse coordinates and the coordinate system as the core readout.
 */
class KadasStatusBar : public QWidget
{
    Q_OBJECT

  public:
    explicit KadasStatusBar( QWidget *parent = nullptr );

    QToolButton *gpsButton() const { return mGpsButton; }
    QLineEdit *coordinateEdit() const { return mCoordinateEdit; }
    QLineEdit *heightEdit() const { return mHeightEdit; }
    QToolButton *displayCrsButton() const { return mDisplayCrsButton; }
    QgsScaleComboBox *scaleCombo() const { return mScaleCombo; }
    QToolButton *scaleLockButton() const { return mScaleLockButton; }
    QgsDoubleSpinBox *magnifierSpinBox() const { return mMagnifierSpinBox; }
    KadasCrsSelection *crsSelectionButton() const { return mCrsSelectionButton; }

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

  protected:
    void resizeEvent( QResizeEvent *event ) override;
    void changeEvent( QEvent *event ) override;

  private:
    //! Progressive collapse levels, richest (0) to most compact.
    enum CollapseLevel
    {
      ShowAll = 0,   //!< Everything visible, including all descriptive labels.
      HideMagnifier, //!< Fold away the magnifier (lowest priority).
      HideScale,     //!< Also fold away the scale controls.
      HideGps,       //!< Also fold away the GPS indicator.
      HideLabels,    //!< Also drop the remaining labels (core values only).
      CollapseLevelCount
    };

    //! Applies the visibility for \a level cumulatively.
    void applyCollapseLevel( CollapseLevel level );
    //! Chooses and applies the richest level that fits the current width.
    void updateResponsiveLayout();
    //! Preferred content width of the layout at its current visibility.
    int currentContentWidth() const;
    //! Caches the full and core preferred widths used as size hints.
    void cacheHintWidths();

    QHBoxLayout *mLayout = nullptr;

    QLabel *mGpsLabel = nullptr;
    QToolButton *mGpsButton = nullptr;
    QLabel *mPositionLabel = nullptr;
    QLineEdit *mCoordinateEdit = nullptr;
    QLineEdit *mHeightEdit = nullptr;
    QToolButton *mDisplayCrsButton = nullptr;
    QLabel *mScaleLabel = nullptr;
    QgsScaleComboBox *mScaleCombo = nullptr;
    QToolButton *mScaleLockButton = nullptr;
    QLabel *mMagnifierLabel = nullptr;
    QgsDoubleSpinBox *mMagnifierSpinBox = nullptr;
    QLabel *mCrsLabel = nullptr;
    KadasCrsSelection *mCrsSelectionButton = nullptr;

    int mFullWidth = 0;
    int mCoreWidth = 0;
    bool mUpdatingLayout = false;
    //! Collapse level currently applied; remembered to add resize hysteresis.
    CollapseLevel mCurrentLevel = ShowAll;
};

#endif // KADASSTATUSBAR_H
