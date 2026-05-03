# The following has been generated automatically from kadas/gui/kadasitemlayer.h
# monkey patching scoped based enum
KadasItemLayer.PICK_OBJECTIVE_ANY = KadasItemLayer.PickObjective.PICK_OBJECTIVE_ANY
KadasItemLayer.PICK_OBJECTIVE_ANY.is_monkey_patched = True
KadasItemLayer.PickObjective.PICK_OBJECTIVE_ANY.__doc__ = ""
KadasItemLayer.PICK_OBJECTIVE_TOOLTIP = KadasItemLayer.PickObjective.PICK_OBJECTIVE_TOOLTIP
KadasItemLayer.PICK_OBJECTIVE_TOOLTIP.is_monkey_patched = True
KadasItemLayer.PickObjective.PICK_OBJECTIVE_TOOLTIP.__doc__ = ""
KadasItemLayer.PickObjective.__doc__ = """

* ``PICK_OBJECTIVE_ANY``: 
* ``PICK_OBJECTIVE_TOOLTIP``: 

"""
# --
# monkey patching scoped based enum
KadasItemLayerRegistry.PinsLayer = KadasItemLayerRegistry.StandardLayer.PinsLayer
KadasItemLayerRegistry.PinsLayer.is_monkey_patched = True
KadasItemLayerRegistry.StandardLayer.PinsLayer.__doc__ = ""
KadasItemLayerRegistry.RoutesLayer = KadasItemLayerRegistry.StandardLayer.RoutesLayer
KadasItemLayerRegistry.RoutesLayer.is_monkey_patched = True
KadasItemLayerRegistry.StandardLayer.RoutesLayer.__doc__ = ""
KadasItemLayerRegistry.StandardLayer.__doc__ = """

* ``PinsLayer``: 
* ``RoutesLayer``: 

"""
# --
try:
    KadasItemLayer.layerType = staticmethod(KadasItemLayer.layerType)
    KadasItemLayer.__overridden_methods__ = ['layerTypeKey', 'clone', 'createMapRenderer', 'extent', 'readXml', 'writeXml']
except (NameError, AttributeError):
    pass
try:
    KadasItemLayerRegistry.getOrCreateItemLayer = staticmethod(KadasItemLayerRegistry.getOrCreateItemLayer)
    KadasItemLayerRegistry.getItemLayers = staticmethod(KadasItemLayerRegistry.getItemLayers)
    KadasItemLayerRegistry.init = staticmethod(KadasItemLayerRegistry.init)
except (NameError, AttributeError):
    pass
try:
    KadasItemLayerType.__overridden_methods__ = ['createLayer', 'addLayerTreeMenuActions']
except (NameError, AttributeError):
    pass
