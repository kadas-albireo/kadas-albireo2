/***************************************************************************
    kadasmaptooldrawshape.h
    -----------------------
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

#ifndef KADASMAPTOOLDRAWSHAPE_H
#define KADASMAPTOOLDRAWSHAPE_H

#include <QPointer>

#include <qgis/qgsdistancearea.h>
#include <qgis/qgsmaptool.h>
#include <qgis/qgspoint.h>

#include <kadas/core/kadasstatestack.h>
#include <kadas/gui/kadas_gui.h>
#include <kadas/gui/kadasgeometryrubberband.h>

class QgsCoordinateReferenceSystem;
class QgsCoordinateTransform;
class KadasFloatingInputWidget;
class KadasFloatingInputWidgetField;

class KADAS_GUI_EXPORT KadasMapToolDrawShape : public QgsMapTool
{
    Q_OBJECT
  public:
    enum Status { StatusReady, StatusDrawing, StatusFinished, StatusEditingReady, StatusEditingMoving };

  protected:
    struct State : KadasStateStack::State
    {
      Status status;
    };
    struct EditContext
    {
      virtual ~EditContext() {}
      bool move = false;
    };

    KadasMapToolDrawShape( QgsMapCanvas* canvas, bool isArea, State* initialState );

  public:
    ~KadasMapToolDrawShape();
    void setParentTool( QgsMapTool* tool ) { mParentTool = tool; }
    void activate() override;
    void deactivate() override;
    void editGeometry( const QgsAbstractGeometry* geometry, const QgsCoordinateReferenceSystem& sourceCrs );
    void setAllowMultipart( bool multipart ) { mMultipart = multipart; }
    void setSnapPoints( bool snapPoints ) { mSnapPoints = snapPoints; }
    void setShowInputWidget( bool showInput ) { mShowInput = showInput; }
    void setResetOnDeactivate( bool resetOnDeactivate ) { mResetOnDeactivate = resetOnDeactivate; }
    void setMeasurementMode( KadasGeometryRubberBand::MeasurementMode measurementMode, QgsUnitTypes::DistanceUnit distanceUnit, QgsUnitTypes::AreaUnit areaUnit, QgsUnitTypes::AngleUnit angleUnit = QgsUnitTypes::AngleDegrees, KadasGeometryRubberBand::AzimuthNorth azimuthNorth = KadasGeometryRubberBand::AZIMUTH_NORTH_GEOGRAPHIC );
    KadasGeometryRubberBand* getRubberBand() const { return mRubberBand; }
    Status getStatus() const { return state()->status; }

    void canvasPressEvent( QgsMapMouseEvent* e ) override;
    void canvasMoveEvent( QgsMapMouseEvent* e ) override;
    void canvasReleaseEvent( QgsMapMouseEvent* e ) override;
    void keyPressEvent( QKeyEvent *e ) override;
    virtual int getPartCount() const = 0;
    virtual QgsAbstractGeometry* createGeometry( const QgsCoordinateReferenceSystem& targetCrs, QList<QgsVertexId>* hiddenNodes = 0 ) const = 0;
    void addGeometry( const QgsAbstractGeometry* geometry, const QgsCoordinateReferenceSystem& sourceCrs );

    virtual void updateStyle( int outlineWidth, const QColor& outlineColor, const QColor& fillColor, Qt::PenStyle lineStyle, Qt::BrushStyle brushStyle );

  public slots:
    void reset();
    void undo() { mStateStack.undo(); }
    void redo() { mStateStack.redo(); }
    void update();

  signals:
    void cleared();
    void finished();
    void geometryChanged();
    void canUndo( bool );
    void canRedo( bool );

  protected:
    bool mIsArea;
    bool mMultipart;
    QPointer<KadasGeometryRubberBand> mRubberBand;
    KadasFloatingInputWidget* mInputWidget;
    KadasStateStack mStateStack;

    const State* state() const { return static_cast<const State*>( mStateStack.state() ); }
    State* mutableState() { return static_cast<State*>( mStateStack.mutableState() ); }
    const EditContext* currentEditContext() const { return mEditContext; }
    virtual State* cloneState() const { return new State( *state() ); }
    virtual State* emptyState() const = 0;
    virtual void buttonEvent( const QgsPointXY& pos, bool press, Qt::MouseButton button ) = 0;
    virtual void moveEvent( const QgsPointXY &/*pos*/ ) { }
    virtual void inputAccepted() { }
    virtual void doAddGeometry( const QgsAbstractGeometry* geometry, const QgsCoordinateTransform& t ) = 0;
    virtual void initInputWidget() {}
    virtual void updateInputWidget( const QgsPointXY& /*mousePos*/ ) {}
    virtual void updateInputWidget( const EditContext* /*context*/ ) {}
    virtual EditContext* getEditContext( const QgsPointXY& pos ) const = 0;
    virtual void edit( const EditContext* context, const QgsPointXY& pos, const QgsVector& delta ) = 0;
    virtual void addContextMenuActions( const EditContext* /*context*/, QMenu& /*menu*/ ) const {}
    virtual void executeContextMenuAction( const EditContext* /*context*/, int /*action*/, const QgsPointXY& /*pos*/ ) {}

    void moveMouseToPos( const QgsPointXY& geoPos );

    static bool pointInPolygon(const QgsPointXY &p, const QList<QgsPointXY> &poly );
    static QgsPointXY projPointOnSegment(const QgsPointXY &p, const QgsPointXY &s1, const QgsPointXY &s2 );

  protected slots:
    void acceptInput();
    void deleteShape();

  private:
    bool mSnapPoints;
    bool mShowInput;
    bool mResetOnDeactivate;
    bool mIgnoreNextMoveEvent;
    QgsPointXY mLastEditPos;
    EditContext* mEditContext;
    QgsMapTool* mParentTool;

    QgsPointXY transformPoint( const QPoint& p );
};

///////////////////////////////////////////////////////////////////////////////

class KADAS_GUI_EXPORT KadasMapToolDrawPoint : public KadasMapToolDrawShape
{
    Q_OBJECT
  public:
    KadasMapToolDrawPoint( QgsMapCanvas* canvas );
    int getPartCount() const override { return state()->points.size(); }
    void getPart( int part, QgsPointXY& p ) const { p = state()->points[part].front(); }
    void setPart( int part, const QgsPoint& p );
    QgsAbstractGeometry* createGeometry( const QgsCoordinateReferenceSystem& targetCrs, QList<QgsVertexId>* hiddenNodes = 0 ) const override;

  protected:
    struct State : KadasMapToolDrawShape::State
    {
      QList< QList<QgsPointXY> > points;
    };
    struct EditContext : KadasMapToolDrawShape::EditContext
    {
      int index;
    };

    QPointer<KadasFloatingInputWidgetField> mXEdit;
    QPointer<KadasFloatingInputWidgetField> mYEdit;

    const State* state() const { return static_cast<const State*>( KadasMapToolDrawShape::state() ); }
    State* mutableState() { return static_cast<State*>( KadasMapToolDrawShape::mutableState() ); }
    State* cloneState() const override { return new State( *state() ); }
    KadasMapToolDrawShape::State* emptyState() const override;
    void buttonEvent( const QgsPointXY& pos, bool press, Qt::MouseButton button ) override;
    void inputAccepted() override;
    void doAddGeometry( const QgsAbstractGeometry* geometry, const QgsCoordinateTransform& t ) override;
    void initInputWidget() override;
    void updateInputWidget( const QgsPointXY& mousePos ) override;
    void updateInputWidget( const KadasMapToolDrawShape::EditContext* context ) override;
    KadasMapToolDrawShape::EditContext* getEditContext( const QgsPointXY& pos ) const override;
    void edit( const KadasMapToolDrawShape::EditContext* context, const QgsPointXY& pos, const QgsVector& delta ) override;

  private slots:
    void inputChanged();
};

///////////////////////////////////////////////////////////////////////////////

class KADAS_GUI_EXPORT KadasMapToolDrawPolyLine : public KadasMapToolDrawShape
{
    Q_OBJECT
  public:
    KadasMapToolDrawPolyLine( QgsMapCanvas* canvas, bool closed, bool geodesic = false );
    int getPartCount() const override { return state()->points.size(); }
    void getPart( int part, QList<QgsPointXY>& p ) const { p = state()->points[part]; }
    void setPart(int part, const QList<QgsPointXY> &p );
    QgsAbstractGeometry* createGeometry( const QgsCoordinateReferenceSystem& targetCrs, QList<QgsVertexId>* hiddenNodes = 0 ) const override;

  protected:
    struct State : KadasMapToolDrawShape::State
    {
      QList< QList<QgsPointXY> > points;
    };
    struct EditContext : KadasMapToolDrawShape::EditContext
    {
      int part;
      int node;
    };

    bool mGeodesic;
    QgsDistanceArea mDa;
    QPointer<KadasFloatingInputWidgetField> mXEdit;
    QPointer<KadasFloatingInputWidgetField> mYEdit;

    const State* state() const { return static_cast<const State*>( KadasMapToolDrawShape::state() ); }
    State* mutableState() { return static_cast<State*>( KadasMapToolDrawShape::mutableState() ); }
    State* cloneState() const override { return new State( *state() ); }
    KadasMapToolDrawShape::State* emptyState() const override;
    void buttonEvent( const QgsPointXY& pos, bool press, Qt::MouseButton button ) override;
    void moveEvent( const QgsPointXY &pos ) override;
    void inputAccepted() override;
    void doAddGeometry( const QgsAbstractGeometry* geometry, const QgsCoordinateTransform& t ) override;
    void initInputWidget() override;
    void updateInputWidget( const QgsPointXY& mousePos ) override;
    void updateInputWidget( const KadasMapToolDrawShape::EditContext* context ) override;
    KadasMapToolDrawShape::EditContext* getEditContext( const QgsPointXY& pos ) const override;
    void edit( const KadasMapToolDrawShape::EditContext* context, const QgsPointXY& pos, const QgsVector& delta ) override;
    void addContextMenuActions( const KadasMapToolDrawShape::EditContext* context, QMenu& menu ) const override;
    void executeContextMenuAction(const KadasMapToolDrawShape::EditContext* context, int action, const QgsPointXY &pos ) override;

  private slots:
    void inputChanged();

  private:
    enum ContextMenuActions {DeleteNode, AddNode, ContinueGeometry};
};

///////////////////////////////////////////////////////////////////////////////

class KADAS_GUI_EXPORT KadasMapToolDrawRectangle : public KadasMapToolDrawShape
{
    Q_OBJECT
  public:
    KadasMapToolDrawRectangle( QgsMapCanvas* canvas );
    int getPartCount() const override { return state()->p1.size(); }
    void getPart( int part, QgsPointXY& p1, QgsPointXY& p2 ) const
    {
      p1 = state()->p1[part];
      p2 = state()->p2[part];
    }
    void setPart( int part, const QgsPoint& p1, const QgsPoint& p2 );
    QgsAbstractGeometry* createGeometry( const QgsCoordinateReferenceSystem& targetCrs, QList<QgsVertexId>* hiddenNodes = 0 ) const override;

  protected:
    struct State : KadasMapToolDrawShape::State
    {
      QList<QgsPointXY> p1, p2;
    };
    struct EditContext : KadasMapToolDrawShape::EditContext
    {
      int part;
      int point;
    };

    QPointer<KadasFloatingInputWidgetField> mXEdit;
    QPointer<KadasFloatingInputWidgetField> mYEdit;

    const State* state() const { return static_cast<const State*>( KadasMapToolDrawShape::state() ); }
    State* mutableState() { return static_cast<State*>( KadasMapToolDrawShape::mutableState() ); }
    State* cloneState() const override { return new State( *state() ); }
    KadasMapToolDrawShape::State* emptyState() const override;
    void buttonEvent( const QgsPointXY& pos, bool press, Qt::MouseButton button ) override;
    void moveEvent( const QgsPointXY &pos ) override;
    void inputAccepted() override;
    void doAddGeometry( const QgsAbstractGeometry* geometry, const QgsCoordinateTransform& t ) override;
    void initInputWidget() override;
    void updateInputWidget( const QgsPointXY& mousePos ) override;
    void updateInputWidget( const KadasMapToolDrawShape::EditContext* context ) override;
    KadasMapToolDrawShape::EditContext* getEditContext( const QgsPointXY& pos ) const override;
    void edit( const KadasMapToolDrawShape::EditContext* context, const QgsPointXY& pos, const QgsVector& delta ) override;

  private slots:
    void inputChanged();
};

///////////////////////////////////////////////////////////////////////////////

class KADAS_GUI_EXPORT KadasMapToolDrawCircle : public KadasMapToolDrawShape
{
    Q_OBJECT
  public:
    KadasMapToolDrawCircle( QgsMapCanvas* canvas, bool geodesic = false );
    int getPartCount() const override { return state()->centers.size(); }
    void getPart(int part, QgsPointXY &center, double& radius ) const;
    void setPart( int part, const QgsPoint& center, double radius );
    QgsAbstractGeometry* createGeometry( const QgsCoordinateReferenceSystem& targetCrs, QList<QgsVertexId>* hiddenNodes = 0 ) const override;

  protected:
    struct State : KadasMapToolDrawShape::State
    {
      QList<QgsPointXY> centers;
      QList<QgsPointXY> ringPos;
    };
    struct EditContext : KadasMapToolDrawShape::EditContext
    {
      int part;
      int point;
    };

    friend class GeodesicCircleMeasurer;
    bool mGeodesic;
    QgsDistanceArea mDa;
    QPointer<KadasFloatingInputWidgetField> mXEdit;
    QPointer<KadasFloatingInputWidgetField> mYEdit;
    QPointer<KadasFloatingInputWidgetField> mREdit;
    mutable QVector<int> mPartMap;

    const State* state() const { return static_cast<const State*>( KadasMapToolDrawShape::state() ); }
    State* mutableState() { return static_cast<State*>( KadasMapToolDrawShape::mutableState() ); }
    State* cloneState() const override { return new State( *state() ); }
    KadasMapToolDrawShape::State* emptyState() const override;
    void buttonEvent( const QgsPointXY& pos, bool press, Qt::MouseButton button ) override;
    void moveEvent( const QgsPointXY &pos ) override;
    void inputAccepted() override;
    void doAddGeometry( const QgsAbstractGeometry* geometry, const QgsCoordinateTransform& t ) override;
    void initInputWidget() override;
    void updateInputWidget( const QgsPointXY& mousePos ) override;
    void updateInputWidget( const KadasMapToolDrawShape::EditContext* context ) override;
    KadasMapToolDrawShape::EditContext* getEditContext( const QgsPointXY& pos ) const override;
    void edit( const KadasMapToolDrawShape::EditContext* context, const QgsPointXY& pos, const QgsVector& delta ) override;

  private slots:
    void centerInputChanged();
    void radiusInputChanged();
};

///////////////////////////////////////////////////////////////////////////////

class KADAS_GUI_EXPORT KadasMapToolDrawCircularSector : public KadasMapToolDrawShape
{
    Q_OBJECT
  public:
    KadasMapToolDrawCircularSector( QgsMapCanvas* canvas );
    int getPartCount() const override { return state()->centers.size(); }
    void getPart( int part, QgsPointXY& center, double& radius, double& startAngle, double& stopAngle ) const
    {
      center = state()->centers[part];
      radius = state()->radii[part];
      startAngle = state()->startAngles[part];
      stopAngle = state()->stopAngles[part];
    }
    void setPart( int part, const QgsPoint& center, double radius, double startAngle, double stopAngle );
    QgsAbstractGeometry* createGeometry( const QgsCoordinateReferenceSystem& targetCrs, QList<QgsVertexId>* hiddenNodes = 0 ) const override;

  protected:
    enum SectorStatus { HaveNothing, HaveCenter, HaveRadius };
    struct State : KadasMapToolDrawShape::State
    {
      SectorStatus sectorStatus;
      QList<QgsPointXY> centers;
      QList<double> radii;
      QList<double> startAngles;
      QList<double> stopAngles;
    };

    QPointer<KadasFloatingInputWidgetField> mXEdit;
    QPointer<KadasFloatingInputWidgetField> mYEdit;
    QPointer<KadasFloatingInputWidgetField> mREdit;
    QPointer<KadasFloatingInputWidgetField> mA1Edit;
    QPointer<KadasFloatingInputWidgetField> mA2Edit;

    const State* state() const { return static_cast<const State*>( KadasMapToolDrawShape::state() ); }
    State* mutableState() { return static_cast<State*>( KadasMapToolDrawShape::mutableState() ); }
    State* cloneState() const override { return new State( *state() ); }
    KadasMapToolDrawShape::State* emptyState() const override;
    void buttonEvent( const QgsPointXY& pos, bool press, Qt::MouseButton button ) override;
    void moveEvent( const QgsPointXY &pos ) override;
    void inputAccepted() override;
    void doAddGeometry( const QgsAbstractGeometry* geometry, const QgsCoordinateTransform& t ) override;
    void initInputWidget() override;
    void updateInputWidget( const QgsPointXY& mousePos ) override;
    void updateInputWidget( const KadasMapToolDrawShape::EditContext* context ) override;
    KadasMapToolDrawShape::EditContext* getEditContext( const QgsPointXY& pos ) const override;
    void edit( const KadasMapToolDrawShape::EditContext* context, const QgsPointXY& pos, const QgsVector& delta ) override;

  private slots:
    void centerInputChanged();
    void arcInputChanged();
};

#endif // KADASMAPTOOLDRAWSHAPE_H
