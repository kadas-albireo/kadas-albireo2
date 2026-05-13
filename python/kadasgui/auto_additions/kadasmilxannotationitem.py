# The following has been generated automatically from kadas/gui/annotationitems/kadasmilxannotationitem.h
# monkey patching scoped based enum
KadasMilxAnnotationItem.DrawStatus.Empty.__doc__ = ""
KadasMilxAnnotationItem.DrawStatus.Drawing.__doc__ = ""
KadasMilxAnnotationItem.DrawStatus.Finished.__doc__ = ""
KadasMilxAnnotationItem.DrawStatus.__doc__ = """

* ``Empty``: 
* ``Drawing``: 
* ``Finished``: 

"""
# --
try:
    KadasMilxAnnotationItem.itemTypeId = staticmethod(KadasMilxAnnotationItem.itemTypeId)
    KadasMilxAnnotationItem.create = staticmethod(KadasMilxAnnotationItem.create)
    KadasMilxAnnotationItem.fromMilx = staticmethod(KadasMilxAnnotationItem.fromMilx)
    KadasMilxAnnotationItem.exportLayerToMilxly = staticmethod(KadasMilxAnnotationItem.exportLayerToMilxly)
    KadasMilxAnnotationItem.importLayerFromMilxly = staticmethod(KadasMilxAnnotationItem.importLayerFromMilxly)
    KadasMilxAnnotationItem.computeScreenExtent = staticmethod(KadasMilxAnnotationItem.computeScreenExtent)
    KadasMilxAnnotationItem.__overridden_methods__ = ['type', 'boundingBox', 'flags', 'render', 'writeXml', 'readXml', 'clone']
except (NameError, AttributeError):
    pass
