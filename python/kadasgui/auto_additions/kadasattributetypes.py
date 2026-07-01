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
# monkey patching scoped based enum
KadasEditContext.HitPrecision.Body.__doc__ = "Loose hit: click is inside the item's filled body / bounding box / containment area."
KadasEditContext.HitPrecision.Precise.__doc__ = "Tight hit: click is on a vertex, handle, or stroke / outline."
KadasEditContext.HitPrecision.__doc__ = """Geometric precision of a hit; a precise (vertex / handle / edge) hit outranks a body hit in picking.

* ``Body``: Loose hit: click is inside the item's filled body / bounding box / containment area.
* ``Precise``: Tight hit: click is on a vertex, handle, or stroke / outline.

"""
# --
try:
    KadasNode.__doc__ = """Editing node descriptor."""
except (NameError, AttributeError):
    pass
try:
    KadasEditContext.__doc__ = """Editing context for an annotation hit."""
except (NameError, AttributeError):
    pass
