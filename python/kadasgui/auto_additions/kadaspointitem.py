# The following has been generated automatically from kadas/gui/mapitems/kadaspointitem.h
try:
    KadasAbstractPointItem.__virtual_methods__ = ['point', 'setPoint']
    KadasAbstractPointItem.__abstract_methods__ = ['setItemGeometry', 'updateQgsAnnotation']
    KadasAbstractPointItem.__overridden_methods__ = ['translate', 'setState', 'startPart', 'setCurrentPoint', 'setCurrentAttributes', 'continuePart', 'endPart', 'intersects', 'nodes', 'getEditContext', 'edit', 'drawAttribs', 'drawAttribsFromPosition', 'positionFromDrawAttribs', 'editAttribsFromPosition', 'positionFromEditAttribs', 'useQgisAnnotations']
except (NameError, AttributeError):
    pass
try:
    KadasPointItem.__overridden_methods__ = ['annotationItem', 'itemName', 'setItemGeometry', 'boundingBox', 'render', '_clone', 'writeXmlPrivate', 'readXmlPrivate']
except (NameError, AttributeError):
    pass
