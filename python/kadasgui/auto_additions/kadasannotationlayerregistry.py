# The following has been generated automatically from kadas/gui/annotationitems/kadasannotationlayerregistry.h
# monkey patching scoped based enum
KadasAnnotationLayerRegistry.StandardLayer.RedliningLayer.__doc__ = ""
KadasAnnotationLayerRegistry.StandardLayer.SymbolsLayer.__doc__ = ""
KadasAnnotationLayerRegistry.StandardLayer.PicturesLayer.__doc__ = ""
KadasAnnotationLayerRegistry.StandardLayer.PinsLayer.__doc__ = ""
KadasAnnotationLayerRegistry.StandardLayer.RoutesLayer.__doc__ = ""
KadasAnnotationLayerRegistry.StandardLayer.MssLayer.__doc__ = ""
KadasAnnotationLayerRegistry.StandardLayer.__doc__ = """

* ``RedliningLayer``: 
* ``SymbolsLayer``: 
* ``PicturesLayer``: 
* ``PinsLayer``: 
* ``RoutesLayer``: 
* ``MssLayer``: 

"""
# --
try:
    KadasAnnotationLayerRegistry.getOrCreateAnnotationLayer = staticmethod(KadasAnnotationLayerRegistry.getOrCreateAnnotationLayer)
    KadasAnnotationLayerRegistry.init = staticmethod(KadasAnnotationLayerRegistry.init)
except (NameError, AttributeError):
    pass
