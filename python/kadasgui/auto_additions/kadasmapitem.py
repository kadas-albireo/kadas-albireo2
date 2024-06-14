# The following has been generated automatically from kadas/gui/mapitems/kadasmapitem.h
KadasMapItem.DrawStatus.DrawStatus = State.DrawStatus
# monkey patching scoped based enum
KadasMapItem.DrawStatus.Empty = State.DrawStatus.Empty
KadasMapItem.DrawStatus.Empty.is_monkey_patched = True
KadasMapItem.DrawStatus.Empty.__doc__ = ""
KadasMapItem.DrawStatus.Drawing = State.DrawStatus.Drawing
KadasMapItem.DrawStatus.Drawing.is_monkey_patched = True
KadasMapItem.DrawStatus.Drawing.__doc__ = ""
KadasMapItem.DrawStatus.Finished = State.DrawStatus.Finished
KadasMapItem.DrawStatus.Finished.is_monkey_patched = True
KadasMapItem.DrawStatus.Finished.__doc__ = ""
State.DrawStatus.__doc__ = "\n\n" + '* ``Empty``: ' + State.DrawStatus.Empty.__doc__ + '\n' + '* ``Drawing``: ' + State.DrawStatus.Drawing.__doc__ + '\n' + '* ``Finished``: ' + State.DrawStatus.Finished.__doc__
# --
KadasMapItem.Type.Type = NumericAttribute.Type
# monkey patching scoped based enum
KadasMapItem.Type.TypeCoordinate = NumericAttribute.Type.TypeCoordinate
KadasMapItem.Type.TypeCoordinate.is_monkey_patched = True
KadasMapItem.Type.TypeCoordinate.__doc__ = ""
KadasMapItem.Type.TypeDistance = NumericAttribute.Type.TypeDistance
KadasMapItem.Type.TypeDistance.is_monkey_patched = True
KadasMapItem.Type.TypeDistance.__doc__ = ""
KadasMapItem.Type.TypeAngle = NumericAttribute.Type.TypeAngle
KadasMapItem.Type.TypeAngle.is_monkey_patched = True
KadasMapItem.Type.TypeAngle.__doc__ = ""
KadasMapItem.Type.TypeOther = NumericAttribute.Type.TypeOther
KadasMapItem.Type.TypeOther.is_monkey_patched = True
KadasMapItem.Type.TypeOther.__doc__ = ""
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
