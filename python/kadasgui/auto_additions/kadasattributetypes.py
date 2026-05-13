# The following has been generated automatically from kadas/gui/kadasattributetypes.h
# monkey patching scoped based enum
KadasNumericAttribute.TypeCoordinate = KadasNumericAttribute.Type.TypeCoordinate
KadasNumericAttribute.TypeCoordinate.is_monkey_patched = True
KadasNumericAttribute.Type.TypeCoordinate.__doc__ = ""
KadasNumericAttribute.TypeDistance = KadasNumericAttribute.Type.TypeDistance
KadasNumericAttribute.TypeDistance.is_monkey_patched = True
KadasNumericAttribute.Type.TypeDistance.__doc__ = ""
KadasNumericAttribute.TypeAngle = KadasNumericAttribute.Type.TypeAngle
KadasNumericAttribute.TypeAngle.is_monkey_patched = True
KadasNumericAttribute.Type.TypeAngle.__doc__ = ""
KadasNumericAttribute.TypeOther = KadasNumericAttribute.Type.TypeOther
KadasNumericAttribute.TypeOther.is_monkey_patched = True
KadasNumericAttribute.Type.TypeOther.__doc__ = ""
KadasNumericAttribute.Type.__doc__ = """

* ``TypeCoordinate``: 
* ``TypeDistance``: 
* ``TypeAngle``: 
* ``TypeOther``: 

"""
# --
try:
    KadasNode.__doc__ = """Editing node descriptor (formerly \c KadasMapItem.Node). The
position is stored as a generic \c :py:class:`QgsPointXY`; legacy callers that
need \c KadasMapPos semantics convert via the implicit constructor."""
except (NameError, AttributeError):
    pass
try:
    KadasEditContext.__doc__ = """Editing context (formerly \c KadasMapItem.EditContext)."""
except (NameError, AttributeError):
    pass
