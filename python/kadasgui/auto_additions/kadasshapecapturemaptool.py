# The following has been generated automatically from kadas/gui/maptools/kadasshapecapturemaptool.h
# monkey patching scoped based enum
KadasShapeCaptureMapTool.Shape.Rectangle.__doc__ = ""
KadasShapeCaptureMapTool.Shape.Circle.__doc__ = ""
KadasShapeCaptureMapTool.Shape.Sector.__doc__ = ""
KadasShapeCaptureMapTool.Shape.Polyline.__doc__ = ""
KadasShapeCaptureMapTool.Shape.Polygon.__doc__ = ""
KadasShapeCaptureMapTool.Shape.__doc__ = """

* ``Rectangle``: 
* ``Circle``: 
* ``Sector``: 
* ``Polyline``: 
* ``Polygon``: 

"""
# --
try:
    KadasShapeCaptureMapTool.__attribute_docs__ = {'previewChanged': 'Emitted whenever the displayed shape preview changes (mouse move,\nnumeric input, programmatic updates).\n'}
    KadasShapeCaptureMapTool.circlePolygon = staticmethod(KadasShapeCaptureMapTool.circlePolygon)
    KadasShapeCaptureMapTool.sectorPolygon = staticmethod(KadasShapeCaptureMapTool.sectorPolygon)
    KadasShapeCaptureMapTool.__overridden_methods__ = ['canvasPressEvent', 'canvasMoveEvent', 'canvasReleaseEvent', 'canvasDoubleClickEvent', 'keyPressEvent', 'activate', 'deactivate']
    KadasShapeCaptureMapTool.__signal_arguments__ = {'shapeCaptured': ['geometry: QgsGeometry', 'crs: QgsCoordinateReferenceSystem']}
except (NameError, AttributeError):
    pass
