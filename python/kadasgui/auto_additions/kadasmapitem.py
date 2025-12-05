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
KadasMapItem.NumericAttribute.TypeOther = KadasMapItem.NumericAttribute.Type.TypeOther
KadasMapItem.NumericAttribute.TypeOther.is_monkey_patched = True
KadasMapItem.NumericAttribute.Type.TypeOther.__doc__ = ""
KadasMapItem.NumericAttribute.Type.__doc__ = """

* ``TypeCoordinate``: 
* ``TypeDistance``: 
* ``TypeAngle``: 
* ``TypeOther``: 

"""
# --
# monkey patching scoped based enum
KadasMapItem.EditNoAction = KadasMapItem.ContextMenuActions.EditNoAction
KadasMapItem.EditNoAction.is_monkey_patched = True
KadasMapItem.ContextMenuActions.EditNoAction.__doc__ = ""
KadasMapItem.EditSwitchToDrawingTool = KadasMapItem.ContextMenuActions.EditSwitchToDrawingTool
KadasMapItem.EditSwitchToDrawingTool.is_monkey_patched = True
KadasMapItem.ContextMenuActions.EditSwitchToDrawingTool.__doc__ = ""
KadasMapItem.ContextMenuActions.__doc__ = """

* ``EditNoAction``: 
* ``EditSwitchToDrawingTool``: 

"""
# --
try:
    KadasMapPos.fromPoint = staticmethod(KadasMapPos.fromPoint)
except AttributeError:
    pass
try:
    KadasItemPos.fromPoint = staticmethod(KadasItemPos.fromPoint)
except AttributeError:
    pass
try:
    KadasMapItem.fromXml = staticmethod(KadasMapItem.fromXml)
    KadasMapItem.defaultNodeRenderer = staticmethod(KadasMapItem.defaultNodeRenderer)
    KadasMapItem.anchorNodeRenderer = staticmethod(KadasMapItem.anchorNodeRenderer)
    KadasMapItem.outputDpiScale = staticmethod(KadasMapItem.outputDpiScale)
    KadasMapItem.getTextRenderScale = staticmethod(KadasMapItem.getTextRenderScale)
    KadasMapItem.__signal_arguments__ = {'zIndexChanged': ['index: int']}
except AttributeError:
    pass
try:
    KadasMapItem.Margin.__doc__ = """Margin in screen units */"""
except AttributeError:
    pass
try:
    KadasMapItem.Node.__doc__ = """Nodes for editing */"""
except AttributeError:
    pass
