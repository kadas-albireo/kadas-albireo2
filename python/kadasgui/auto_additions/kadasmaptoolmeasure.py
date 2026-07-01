# The following has been generated automatically from kadas/gui/maptools/kadasmaptoolmeasure.h
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
# monkey patching scoped based enum
KadasMapToolMeasure.AzimuthNorth.AzimuthMapNorth.__doc__ = ""
KadasMapToolMeasure.AzimuthNorth.AzimuthGeoNorth.__doc__ = ""
KadasMapToolMeasure.AzimuthNorth.__doc__ = """

* ``AzimuthMapNorth``: 
* ``AzimuthGeoNorth``: 

"""
# --
KadasMapToolMeasure.AzimuthNorth.baseClass = KadasMapToolMeasure
try:
    KadasMapToolMeasure.__overridden_methods__ = ['activate', 'deactivate', 'canvasPressEvent', 'canvasReleaseEvent', 'keyReleaseEvent']
except (NameError, AttributeError):
    pass
