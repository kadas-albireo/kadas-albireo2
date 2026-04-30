


#include <qgis/qgsstyle.h>
#include <qgis/qgsannotationlayer3drenderer.h>
#include <qgis/qgs3dmapsettings.h>
#include <qgis/qgsannotationlayer.h>

#include "kadasmapitemlayer3drenderer.h"
#include "kadas/gui/kadasitemlayer.h"


//
// KadasMapItemLayer3DRendererMetadata
//

KadasMapItemLayer3DRendererMetadata::KadasMapItemLayer3DRendererMetadata()
  : Qgs3DRendererAbstractMetadata( QStringLiteral( "mapitem" ) )
{}

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
  return QStringLiteral( "mapitem" );
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

  // qgisAnnotationLayer() transfers ownership; the QgsAnnotationLayerChunkLoaderFactory only
  // stores a raw pointer to it, so we must keep the layer alive for the entity's lifetime.
  // Parent it to the entity below so it is destroyed together with it.
  QgsAnnotationLayer *annotationLayer = itemLayer->qgisAnnotationLayer( map->crs() );

  // Delegate the actual entity construction to the public QgsAnnotationLayer3DRenderer,
  // since QgsAnnotationLayerChunkedEntity lives in a private header and is not exported.
  QgsAnnotationLayer3DRenderer renderer;
  renderer.setLayer( annotationLayer );
  renderer.setAltitudeClamping( mAltClamping );
  renderer.setZOffset( mZOffset );
  renderer.setShowCalloutLines( mShowCalloutLines );
  renderer.setCalloutLineColor( mCalloutLineColor );
  renderer.setCalloutLineWidth( mCalloutLineWidth );
  renderer.setTextFormat( mTextFormat );

  Qt3DCore::QEntity *entity = renderer.createEntity( map );
  if ( !entity )
  {
    delete annotationLayer;
    return nullptr;
  }
  // Tie the ephemeral annotation layer's lifetime to the entity.
  annotationLayer->setParent( entity );
  return entity;
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
