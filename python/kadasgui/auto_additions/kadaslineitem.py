# The following has been generated automatically from kadas/gui/mapitems/kadaslineitem.h
# monkey patching scoped based enum
KadasLineItem.MeasureLineAndSegments = KadasLineItem.MeasurementMode.MeasureLineAndSegments
KadasLineItem.MeasureLineAndSegments.is_monkey_patched = True
KadasLineItem.MeasurementMode.MeasureLineAndSegments.__doc__ = ""
KadasLineItem.MeasureAzimuthMapNorth = KadasLineItem.MeasurementMode.MeasureAzimuthMapNorth
KadasLineItem.MeasureAzimuthMapNorth.is_monkey_patched = True
KadasLineItem.MeasurementMode.MeasureAzimuthMapNorth.__doc__ = ""
KadasLineItem.MeasureAzimuthGeoNorth = KadasLineItem.MeasurementMode.MeasureAzimuthGeoNorth
KadasLineItem.MeasureAzimuthGeoNorth.is_monkey_patched = True
KadasLineItem.MeasurementMode.MeasureAzimuthGeoNorth.__doc__ = ""
KadasLineItem.MeasureLineAndSegmentsAndAzimuthMapNorth = KadasLineItem.MeasurementMode.MeasureLineAndSegmentsAndAzimuthMapNorth
KadasLineItem.MeasureLineAndSegmentsAndAzimuthMapNorth.is_monkey_patched = True
KadasLineItem.MeasurementMode.MeasureLineAndSegmentsAndAzimuthMapNorth.__doc__ = ""
KadasLineItem.MeasureLineAndSegmentsAndAzimuthGeoNorth = KadasLineItem.MeasurementMode.MeasureLineAndSegmentsAndAzimuthGeoNorth
KadasLineItem.MeasureLineAndSegmentsAndAzimuthGeoNorth.is_monkey_patched = True
KadasLineItem.MeasurementMode.MeasureLineAndSegmentsAndAzimuthGeoNorth.__doc__ = ""
KadasLineItem.MeasurementMode.__doc__ = """

* ``MeasureLineAndSegments``: 
* ``MeasureAzimuthMapNorth``: 
* ``MeasureAzimuthGeoNorth``: 
* ``MeasureLineAndSegmentsAndAzimuthMapNorth``: 
* ``MeasureLineAndSegmentsAndAzimuthGeoNorth``: 

"""
# --
try:
    KadasLineItem.__overridden_methods__ = ['itemName', 'nodes', 'startPart', 'setCurrentPoint', 'setCurrentAttributes', 'continuePart', 'endPart', 'drawAttribs', 'drawAttribsFromPosition', 'positionFromDrawAttribs', 'getEditContext', 'edit', 'populateContextMenu', 'editAttribsFromPosition', 'positionFromEditAttribs', 'position', 'setPosition', 'geometryType', 'addPartFromGeometry', 'assign', 'clone', 'serialize', 'deserialize', '_clone', 'createEmptyState', 'recomputeDerived', 'measureGeometry']
except (NameError, AttributeError):
    pass
