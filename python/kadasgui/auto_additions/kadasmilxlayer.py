# The following has been generated automatically from kadas/gui/milx/kadasmilxlayer.h
try:
    KadasMilxLayer.layerType = staticmethod(KadasMilxLayer.layerType)
    KadasMilxLayer.__overridden_methods__ = ['layerTypeKey', 'acceptsItem', 'clone', 'readXml', 'writeXml', 'createMapRenderer', 'pickItem']
    KadasMilxLayer.__signal_arguments__ = {'approvedChanged': ['approved: bool']}
except (NameError, AttributeError):
    pass
try:
    KadasMilxLayerType.__overridden_methods__ = ['createLayer', 'addLayerTreeMenuActions']
except (NameError, AttributeError):
    pass
