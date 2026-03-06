# The following has been generated automatically from kadas/gui/maptools/kadasmaptoolminmax.h
# monkey patching scoped based enum
KadasMapToolMinMax.FilterType.FilterRect.__doc__ = ""
KadasMapToolMinMax.FilterType.FilterPoly.__doc__ = ""
KadasMapToolMinMax.FilterType.FilterCircle.__doc__ = ""
KadasMapToolMinMax.FilterType.__doc__ = """

* ``FilterRect``: 
* ``FilterPoly``: 
* ``FilterCircle``: 

"""
# --
KadasMapToolMinMax.FilterType.baseClass = KadasMapToolMinMax
try:
    KadasMapToolMinMax.__overridden_methods__ = ['canvasPressEvent', 'canvasMoveEvent', 'canvasReleaseEvent']
except (NameError, AttributeError):
    pass
try:
    KadasMapToolMinMaxItemInterface.__overridden_methods__ = ['createItem']
except (NameError, AttributeError):
    pass
