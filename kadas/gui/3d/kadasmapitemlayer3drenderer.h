#ifndef KADASMAPITEMLAYER3DRENDERER_H
#define KADASMAPITEMLAYER3DRENDERER_H


#include <qgis/qgs3drendererregistry.h>
#include <qgis/qgsabstract3drenderer.h>
#include <qgis/qgstextformat.h>
#include <qgis/qgsmaplayerref.h>

#include "kadas/gui/kadas_gui.h"

class KadasItemLayer;


#ifdef SIP_RUN
// this is needed for the "convert to subclass" code below to compile
% ModuleHeaderCode
#include "kadas/gui/3d/kadasmapitemlayer3drenderer.h"
  % End
#endif


  // This code has been copied from QgsAnnotationLayer3DRenderer

  /**
 * \ingroup core
 * \brief Metadata for annotation layer 3D renderer to allow creation of its instances from XML.
 *
 * \warning This is not considered stable API, and may change in future QGIS releases. It is
 * exposed to the Python bindings as a tech preview only.
 *
 * \since QGIS 4.0
 */
  class KADAS_GUI_EXPORT KadasMapItemLayer3DRendererMetadata : public Qgs3DRendererAbstractMetadata
{
  public:
    KadasMapItemLayer3DRendererMetadata();

    //! Creates an instance of a 3D renderer based on a DOM element with renderer configuration
    QgsAbstract3DRenderer *createRenderer( QDomElement &elem, const QgsReadWriteContext &context ) override SIP_FACTORY;
};

/**
 * \ingroup qgis_3d
 * \brief 3D renderers for annotation layers.
 *
 * \since QGIS 4.0
 */
class KADAS_GUI_EXPORT KadasMapItemLayer3DRenderer : public QgsAbstract3DRenderer
{
#ifdef SIP_RUN
    SIP_CONVERT_TO_SUBCLASS_CODE
    if ( dynamic_cast<KadasMapItemLayer3DRenderer *>( sipCpp ) != nullptr )
      sipType = sipType_KadasMapItemLayer3DRenderer;
    else
      sipType = nullptr;
    SIP_END
#endif


  public:
    KadasMapItemLayer3DRenderer();

    void setLayer( KadasItemLayer *layer );


    QString type() const override;
    KadasMapItemLayer3DRenderer *clone() const override SIP_FACTORY;
    Qt3DCore::QEntity *createEntity( Qgs3DMapSettings *map ) const override SIP_SKIP;

    void writeXml( QDomElement &elem, const QgsReadWriteContext &context ) const override;
    void readXml( const QDomElement &elem, const QgsReadWriteContext &context ) override;
    void resolveReferences( const QgsProject &project ) override;

    /**
     * Returns the altitude clamping method, which determines the vertical position of annotations.
     *
     * \see setAltitudeClamping()
     */
    Qgis::AltitudeClamping altitudeClamping() const { return mAltClamping; }

    /**
     * Sets the altitude \a clamping method, which determines the vertical position of annotations.
     *
     * \see altitudeClamping()
     */
    void setAltitudeClamping( Qgis::AltitudeClamping clamping ) { mAltClamping = clamping; }

    /**
     * Returns the z offset, which is a fixed offset amount which should be added to z values for the annotations.
     *
     * \see setZOffset()
     */
    double zOffset() const { return mZOffset; }

    /**
     * Sets the z \a offset, which is a fixed offset amount which will be added to z values for the annotations.
     *
     * \see zOffset()
     */
    void setZOffset( double offset ) { mZOffset = offset; }

    /**
     * Returns TRUE if callout lines are shown, vertically joining the annotations to the terrain.
     *
     * \see setShowCalloutLines()
     */
    bool showCalloutLines() const;

    /**
     * Sets whether callout lines are shown, vertically joining the annotations to the terrain.
     *
     * \see showCalloutLines()
     */
    void setShowCalloutLines( bool show );

    // TODO -- consider exposing via QgsSimpleLineMaterialSettings, for now, for testing only
    /**
     * Sets the callout line \a color.
     *
     * \see calloutLineColor()
     * \note Not available in Python bindings
     */
    SIP_SKIP void setCalloutLineColor( const QColor &color );

    /**
     * Returns the callout line color.
     *
     * \see setCalloutLineColor()
     * \note Not available in Python bindings
     */
    SIP_SKIP QColor calloutLineColor() const;

    /**
     * Sets the callout line \a width.
     *
     * \see calloutLineWidth()
     * \note Not available in Python bindings
     */
    SIP_SKIP void setCalloutLineWidth( double width );

    /**
     * Returns the callout line width.
     *
     * \see setCalloutLineWidth()
     * \note Not available in Python bindings
     */
    SIP_SKIP double calloutLineWidth() const;

    /**
     * Returns the text format to use for rendering text annotations in 3D.
     *
     * \see setTextFormat()
     */
    QgsTextFormat textFormat() const;

    /**
     * Sets the text \a format to use for rendering text annotations in 3D.
     *
     * \see textFormat()
     */
    void setTextFormat( const QgsTextFormat &format );

  private:
#ifdef SIP_RUN
    KadasMapItemLayer3DRenderer( const KadasMapItemLayer3DRenderer & );
#endif

    static constexpr double DEFAULT_Z_OFFSET = 50;

    QgsMapLayerRef mLayerRef;
    Qgis::AltitudeClamping mAltClamping = Qgis::AltitudeClamping::Relative;
    double mZOffset = DEFAULT_Z_OFFSET;
    bool mShowCalloutLines = true;
    QColor mCalloutLineColor { 0, 0, 0 };
    double mCalloutLineWidth = 2;
    QgsTextFormat mTextFormat;
};

#endif // KADASMAPITEMLAYER3DRENDERER
