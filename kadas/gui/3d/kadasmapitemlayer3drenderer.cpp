


#include <qgis/qgsstyle.h>
#include <qgis/qgsannotationlayerchunkloader_p.h>

#include "kadasmapitemlayer3drenderer.h"
#include "kadas/gui/kadasitemlayer.h"


//
// KadasMapItemLayer3DRendererMetadata
//

KadasMapItemLayer3DRendererMetadata::KadasMapItemLayer3DRendererMetadata()
  : Qgs3DRendererAbstractMetadata( QStringLiteral( "mapitem" ) )
{
}

QgsAbstract3DRenderer *KadasMapItemLayer3DRendererMetadata::createRenderer( QDomElement &elem, const QgsReadWriteContext &context )
{
  auto r = std::make_unique<KadasMapItemLayer3DRenderer>();
  r->readXml( elem, context );
  return r.release();
}


//
// KadasMapItemLayer3DRenderer
//

KadasMapItemLayer3DRenderer::KadasMapItemLayer3DRenderer()
{
  mTextFormat = QgsStyle::defaultStyle()->defaultTextFormat();
  mTextFormat.setNamedStyle( QStringLiteral( "Bold" ) );
  mTextFormat.setSize( 20 );
}

void KadasMapItemLayer3DRenderer::setLayer( KadasItemLayer *layer )
{
  mLayerRef.layer = layer;
}

void KadasMapItemLayer3DRenderer::resolveReferences( const QgsProject &project )
{
  mLayerRef.resolve( &project );
}

bool KadasMapItemLayer3DRenderer::showCalloutLines() const
{
  return mShowCalloutLines;
}

void KadasMapItemLayer3DRenderer::setShowCalloutLines( bool show )
{
  mShowCalloutLines = show;
}

void KadasMapItemLayer3DRenderer::setCalloutLineColor( const QColor &color )
{
  mCalloutLineColor = color;
}

QColor KadasMapItemLayer3DRenderer::calloutLineColor() const
{
  return mCalloutLineColor;
}

void KadasMapItemLayer3DRenderer::setCalloutLineWidth( double width )
{
  mCalloutLineWidth = width;
}

double KadasMapItemLayer3DRenderer::calloutLineWidth() const
{
  return mCalloutLineWidth;
}

QgsTextFormat KadasMapItemLayer3DRenderer::textFormat() const
{
  return mTextFormat;
}

void KadasMapItemLayer3DRenderer::setTextFormat( const QgsTextFormat &format )
{
  mTextFormat = format;
}

QString KadasMapItemLayer3DRenderer::type() const
{
  return "annotation";
}

KadasMapItemLayer3DRenderer *KadasMapItemLayer3DRenderer::clone() const
{
  auto r = std::make_unique<KadasMapItemLayer3DRenderer>();
  r->mLayerRef = mLayerRef;
  r->mAltClamping = mAltClamping;
  r->mZOffset = mZOffset;
  r->mShowCalloutLines = mShowCalloutLines;
  r->mCalloutLineColor = mCalloutLineColor;
  r->mCalloutLineWidth = mCalloutLineWidth;
  r->mTextFormat = mTextFormat;
  return r.release();
}

Qt3DCore::QEntity *KadasMapItemLayer3DRenderer::createEntity( Qgs3DMapSettings *map ) const
{
  KadasItemLayer *itemLayer = qobject_cast<KadasItemLayer *>( mLayerRef.layer );

  if ( !itemLayer )
    return nullptr;

  QgsAnnotationLayer *annotationLayer = itemLayer->qgisAnnotationLayer();

  // For some cases we start with a maximal z range because we can't know this upfront, as it potentially involves terrain heights.
  // This range will be refined after populating the nodes to the actual z range of the generated chunks nodes.
  // Assuming the vertical height is in meter, then it's extremely unlikely that a real vertical
  // height will exceed this amount!
  constexpr double MINIMUM_ANNOTATION_Z_ESTIMATE = -100000;
  constexpr double MAXIMUM_ANNOTATION_Z_ESTIMATE = 100000;

  double minimumZ = MINIMUM_ANNOTATION_Z_ESTIMATE;
  double maximumZ = MAXIMUM_ANNOTATION_Z_ESTIMATE;
  switch ( mAltClamping )
  {
    case Qgis::AltitudeClamping::Absolute:
      // special case where we DO know the exact z range upfront!
      minimumZ = mZOffset;
      maximumZ = mZOffset;
      break;

    case Qgis::AltitudeClamping::Relative:
    case Qgis::AltitudeClamping::Terrain:
      break;
  }

  return new QgsAnnotationLayerChunkedEntity( map, annotationLayer, mAltClamping, mZOffset, mShowCalloutLines, mCalloutLineColor, mCalloutLineWidth, mTextFormat, minimumZ, maximumZ );
}

void KadasMapItemLayer3DRenderer::writeXml( QDomElement &elem, const QgsReadWriteContext &context ) const
{
  QDomDocument doc = elem.ownerDocument();

  elem.setAttribute( QStringLiteral( "layer" ), mLayerRef.layerId );
  elem.setAttribute( QStringLiteral( "clamping" ), qgsEnumValueToKey( mAltClamping ) );
  elem.setAttribute( QStringLiteral( "offset" ), mZOffset );
  if ( mShowCalloutLines )
    elem.setAttribute( QStringLiteral( "callouts" ), QStringLiteral( "1" ) );

  if ( mTextFormat.isValid() )
  {
    elem.appendChild( mTextFormat.writeXml( doc, context ) );
  }
}

void KadasMapItemLayer3DRenderer::readXml( const QDomElement &elem, const QgsReadWriteContext &context )
{
  mLayerRef = QgsMapLayerRef( elem.attribute( QStringLiteral( "layer" ) ) );
  mAltClamping = qgsEnumKeyToValue( elem.attribute( QStringLiteral( "clamping" ) ), Qgis::AltitudeClamping::Relative );
  mZOffset = elem.attribute( QStringLiteral( "offset" ), QString::number( DEFAULT_Z_OFFSET ) ).toDouble();
  mShowCalloutLines = elem.attribute( QStringLiteral( "callouts" ), QStringLiteral( "0" ) ).toInt();
  if ( !elem.firstChildElement( QStringLiteral( "text-style" ) ).isNull() )
  {
    mTextFormat = QgsTextFormat();
    mTextFormat.readXml( elem.firstChildElement( QStringLiteral( "text-style" ) ), context );
  }
}
