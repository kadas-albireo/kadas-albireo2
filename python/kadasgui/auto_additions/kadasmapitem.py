# The following has been generated automatically from kadas/gui/mapitems/kadasmapitem.h
# monkey patching scoped based enum
State.Empty = State.DrawStatus.Empty
State.Empty.is_monkey_patched = True
KadasMapItem.State.DrawStatus.Empty.__doc__ = ""
State.Drawing = State.DrawStatus.Drawing
State.Drawing.is_monkey_patched = True
KadasMapItem.State.DrawStatus.Drawing.__doc__ = ""
State.Finished = State.DrawStatus.Finished
State.Finished.is_monkey_patched = True
KadasMapItem.State.DrawStatus.Finished.__doc__ = ""
State.DrawStatus.__doc__ = "\n\n" + '* ``Empty``: ' + State.DrawStatus.Empty.__doc__ + '\n' + '* ``Drawing``: ' + State.DrawStatus.Drawing.__doc__ + '\n' + '* ``Finished``: ' + State.DrawStatus.Finished.__doc__
# --
# monkey patching scoped based enum
NumericAttribute.TypeCoordinate = NumericAttribute.Type.TypeCoordinate
NumericAttribute.TypeCoordinate.is_monkey_patched = True
KadasMapItem.NumericAttribute.Type.TypeCoordinate.__doc__ = ""
NumericAttribute.TypeDistance = NumericAttribute.Type.TypeDistance
NumericAttribute.TypeDistance.is_monkey_patched = True
KadasMapItem.NumericAttribute.Type.TypeDistance.__doc__ = ""
NumericAttribute.TypeAngle = NumericAttribute.Type.TypeAngle
NumericAttribute.TypeAngle.is_monkey_patched = True
KadasMapItem.NumericAttribute.Type.TypeAngle.__doc__ = ""
NumericAttribute.TypeOther = NumericAttribute.Type.TypeOther
NumericAttribute.TypeOther.is_monkey_patched = True
KadasMapItem.NumericAttribute.Type.TypeOther.__doc__ = ""
NumericAttribute.Type.__doc__ = "\n\n" + '* ``TypeCoordinate``: ' + NumericAttribute.Type.TypeCoordinate.__doc__ + '\n' + '* ``TypeDistance``: ' + NumericAttribute.Type.TypeDistance.__doc__ + '\n' + '* ``TypeAngle``: ' + NumericAttribute.Type.TypeAngle.__doc__ + '\n' + '* ``TypeOther``: ' + NumericAttribute.Type.TypeOther.__doc__
# --
# monkey patching scoped based enum
KadasMapItem.EditNoAction = KadasMapItem.ContextMenuActions.EditNoAction
KadasMapItem.EditNoAction.is_monkey_patched = True
KadasMapItem.ContextMenuActions.EditNoAction.__doc__ = ""
KadasMapItem.EditSwitchToDrawingTool = KadasMapItem.ContextMenuActions.EditSwitchToDrawingTool
KadasMapItem.EditSwitchToDrawingTool.is_monkey_patched = True
KadasMapItem.ContextMenuActions.EditSwitchToDrawingTool.__doc__ = ""
KadasMapItem.ContextMenuActions.__doc__ = "\n\n" + '* ``EditNoAction``: ' + KadasMapItem.ContextMenuActions.EditNoAction.__doc__ + '\n' + '* ``EditSwitchToDrawingTool``: ' + KadasMapItem.ContextMenuActions.EditSwitchToDrawingTool.__doc__
# --
