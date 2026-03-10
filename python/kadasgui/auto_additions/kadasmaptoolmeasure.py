# The following has been generated automatically from kadas/gui/maptools/kadasmaptoolmeasure.h
# monkey patching scoped based enum
KadasMeasureWidget.AzimuthNorth.AzimuthMapNorth.__doc__ = ""
KadasMeasureWidget.AzimuthNorth.AzimuthGeoNorth.__doc__ = ""
KadasMeasureWidget.AzimuthNorth.__doc__ = """

* ``AzimuthMapNorth``: 
* ``AzimuthGeoNorth``: 

"""
# --
KadasMeasureWidget.AzimuthNorth.baseClass = KadasMeasureWidget
# monkey patching scoped based enum
KadasMapToolMeasure.MeasureMode.MeasureLine.__doc__ = ""
KadasMapToolMeasure.MeasureMode.MeasurePolygon.__doc__ = ""
KadasMapToolMeasure.MeasureMode.MeasureCircle.__doc__ = ""
KadasMapToolMeasure.MeasureMode.__doc__ = """

* ``MeasureLine``: 
* ``MeasurePolygon``: 
* ``MeasureCircle``: 

"""
# --
try:
    KadasMeasureWidget.__overridden_methods__ = ['syncItemToWidget', 'syncWidgetToItem', 'setItem']
except (NameError, AttributeError):
    pass
try:
    KadasMapToolMeasure.__overridden_methods__ = ['activate', 'canvasPressEvent', 'canvasMoveEvent', 'canvasReleaseEvent', 'keyReleaseEvent']
except (NameError, AttributeError):
    pass
try:
    KadasMapToolMeasureItemInterface.__overridden_methods__ = ['createItem']
except (NameError, AttributeError):
    pass
