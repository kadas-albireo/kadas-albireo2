# The following has been generated automatically from kadas/gui/mapitems/kadasmapitem.h
# monkey patching scoped based enum
State.Empty = KadasMapItem.State.DrawStatus.Empty
State.Empty.is_monkey_patched = True
KadasMapItem.State.DrawStatus.Empty.__doc__ = ""
State.Drawing = KadasMapItem.State.DrawStatus.Drawing
State.Drawing.is_monkey_patched = True
KadasMapItem.State.DrawStatus.Drawing.__doc__ = ""
State.Finished = KadasMapItem.State.DrawStatus.Finished
State.Finished.is_monkey_patched = True
KadasMapItem.State.DrawStatus.Finished.__doc__ = ""
State.DrawStatus.__doc__ = """

* ``Empty``: 
* ``Drawing``: 
* ``Finished``: 

"""
# --
# monkey patching scoped based enum
NumericAttribute.TypeCoordinate = KadasMapItem.NumericAttribute.Type.TypeCoordinate
NumericAttribute.TypeCoordinate.is_monkey_patched = True
KadasMapItem.NumericAttribute.Type.TypeCoordinate.__doc__ = ""
NumericAttribute.TypeDistance = KadasMapItem.NumericAttribute.Type.TypeDistance
NumericAttribute.TypeDistance.is_monkey_patched = True
KadasMapItem.NumericAttribute.Type.TypeDistance.__doc__ = ""
NumericAttribute.TypeAngle = KadasMapItem.NumericAttribute.Type.TypeAngle
NumericAttribute.TypeAngle.is_monkey_patched = True
KadasMapItem.NumericAttribute.Type.TypeAngle.__doc__ = ""
NumericAttribute.TypeOther = KadasMapItem.NumericAttribute.Type.TypeOther
NumericAttribute.TypeOther.is_monkey_patched = True
KadasMapItem.NumericAttribute.Type.TypeOther.__doc__ = ""
NumericAttribute.Type.__doc__ = """

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
except NameError:
    pass
try:
    KadasItemPos.fromPoint = staticmethod(KadasItemPos.fromPoint)
except NameError:
    pass
try:
    KadasMapItem.fromXml = staticmethod(KadasMapItem.fromXml)
    KadasMapItem.defaultNodeRenderer = staticmethod(KadasMapItem.defaultNodeRenderer)
    KadasMapItem.anchorNodeRenderer = staticmethod(KadasMapItem.anchorNodeRenderer)
    KadasMapItem.outputDpiScale = staticmethod(KadasMapItem.outputDpiScale)
except NameError:
    pass
try:
    KadasMapItem.Margin.__doc__ = """Margin in screen units */"""
except NameError:
    pass
try:
    KadasMapItem.Node.__doc__ = """Nodes for editing */"""
except NameError:
    pass
