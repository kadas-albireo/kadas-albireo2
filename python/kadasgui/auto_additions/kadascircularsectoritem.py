# The following has been generated automatically from kadas/gui/mapitems/kadascircularsectoritem.h
# monkey patching scoped based enum
KadasCircularSectorItem.State.SectorStatus.HaveNothing.__doc__ = ""
KadasCircularSectorItem.State.SectorStatus.HaveCenter.__doc__ = ""
KadasCircularSectorItem.State.SectorStatus.HaveRadius.__doc__ = ""
KadasCircularSectorItem.State.SectorStatus.__doc__ = """

* ``HaveNothing``: 
* ``HaveCenter``: 
* ``HaveRadius``: 

"""
# --
try:
    KadasCircularSectorItem.__overridden_methods__ = ['itemName', 'nodes', 'startPart', 'setCurrentPoint', 'setCurrentAttributes', 'continuePart', 'endPart', 'drawAttribs', 'drawAttribsFromPosition', 'positionFromDrawAttribs', 'getEditContext', 'edit', 'editAttribsFromPosition', 'positionFromEditAttribs', 'addPartFromGeometry', 'geometryType', 'position', 'setPosition', '_clone', 'createEmptyState', 'recomputeDerived', 'measureGeometry']
except (NameError, AttributeError):
    pass
try:
    KadasCircularSectorItem.State.__overridden_methods__ = ['assign', 'clone', 'serialize', 'deserialize']
except (NameError, AttributeError):
    pass
