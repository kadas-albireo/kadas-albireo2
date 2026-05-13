# The following has been generated automatically from kadas/gui/mapitems/kadasmapitem.h
# monkey patching scoped based enum
KadasMapItem.DrawStatus.Empty.__doc__ = ""
KadasMapItem.DrawStatus.Drawing.__doc__ = ""
KadasMapItem.DrawStatus.Finished.__doc__ = ""
KadasMapItem.DrawStatus.__doc__ = """

* ``Empty``: 
* ``Drawing``: 
* ``Finished``: 

"""
# --
KadasMapItem.DrawStatus.baseClass = KadasMapItem
# monkey patching scoped based enum
KadasMapItem.NumericAttribute.TypeCoordinate = KadasMapItem.NumericAttribute.Type.TypeCoordinate
KadasMapItem.NumericAttribute.TypeCoordinate.is_monkey_patched = True
KadasMapItem.NumericAttribute.Type.TypeCoordinate.__doc__ = ""
KadasMapItem.NumericAttribute.TypeDistance = KadasMapItem.NumericAttribute.Type.TypeDistance
KadasMapItem.NumericAttribute.TypeDistance.is_monkey_patched = True
KadasMapItem.NumericAttribute.Type.TypeDistance.__doc__ = ""
KadasMapItem.NumericAttribute.TypeAngle = KadasMapItem.NumericAttribute.Type.TypeAngle
KadasMapItem.NumericAttribute.TypeAngle.is_monkey_patched = True
KadasMapItem.NumericAttribute.Type.TypeAngle.__doc__ = ""
KadasMapItem.NumericAttribute.Type.__doc__ = """

* ``TypeCoordinate``: 
* ``TypeDistance``: 
* ``TypeAngle``: 

"""
# --
# monkey patching scoped based enum
KadasMapItem.EditSwitchToDrawingTool = KadasMapItem.ContextMenuActions.EditSwitchToDrawingTool
KadasMapItem.EditSwitchToDrawingTool.is_monkey_patched = True
KadasMapItem.ContextMenuActions.EditSwitchToDrawingTool.__doc__ = ""
KadasMapItem.ContextMenuActions.__doc__ = """

* ``EditSwitchToDrawingTool``: 

"""
# --
try:
    KadasMapPos.fromPoint = staticmethod(KadasMapPos.fromPoint)
except (NameError, AttributeError):
    pass
try:
    KadasItemPos.fromPoint = staticmethod(KadasItemPos.fromPoint)
except (NameError, AttributeError):
    pass
try:
    KadasMapItem.fromXml = staticmethod(KadasMapItem.fromXml)
    KadasMapItem.defaultNodeRenderer = staticmethod(KadasMapItem.defaultNodeRenderer)
    KadasMapItem.anchorNodeRenderer = staticmethod(KadasMapItem.anchorNodeRenderer)
    KadasMapItem.outputDpiScale = staticmethod(KadasMapItem.outputDpiScale)
    KadasMapItem.getTextRenderScale = staticmethod(KadasMapItem.getTextRenderScale)
    KadasMapItem.__virtual_methods__ = ['useQgisAnnotations', 'annotationItem', 'annotationItems', 'exportName', 'margin', 'hitTest', 'closestPoint', 'setState', 'clear', 'populateContextMenu', 'onDoubleClick', 'position', 'setPosition', 'translate', 'createEmptyState', 'writeXmlPrivate', 'readXmlPrivate']
    KadasMapItem.__abstract_methods__ = ['itemName', 'boundingBox', 'nodes', 'intersects', 'render', 'startPart', 'setCurrentPoint', 'setCurrentAttributes', 'continuePart', 'endPart', 'drawAttribs', 'drawAttribsFromPosition', 'positionFromDrawAttribs', 'getEditContext', 'edit', 'editAttribsFromPosition', 'positionFromEditAttribs']
    KadasMapItem.__signal_arguments__ = {'zIndexChanged': ['index: int']}
except (NameError, AttributeError):
    pass
try:
    KadasMapItem.State.__virtual_methods__ = ['deserialize']
    KadasMapItem.State.__abstract_methods__ = ['assign', 'clone']
except (NameError, AttributeError):
    pass
try:
    KadasMapItem.Margin.__doc__ = """Margin in screen units */"""
except (NameError, AttributeError):
    pass
try:
    KadasMapItem.Node.__doc__ = """Nodes for editing */"""
except (NameError, AttributeError):
    pass
