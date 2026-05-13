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
try:
    KadasItemLayer.layerType = staticmethod(KadasItemLayer.layerType)
    KadasItemLayer.__virtual_methods__ = ['pickItem']
    KadasItemLayer.__overridden_methods__ = ['layerTypeKey', 'clone', 'createMapRenderer', 'extent', 'readXml', 'writeXml']
except (NameError, AttributeError):
    pass
try:
    KadasItemLayerRegistry.getItemLayers = staticmethod(KadasItemLayerRegistry.getItemLayers)
except (NameError, AttributeError):
    pass
try:
    KadasItemLayerType.__overridden_methods__ = ['createLayer', 'addLayerTreeMenuActions']
except (NameError, AttributeError):
    pass
