# The following has been generated automatically from kadas/gui/annotationitems/kadasannotationitemcontroller.h
try:
    KadasAnnotationItemController.toMapPos = staticmethod(KadasAnnotationItemController.toMapPos)
    KadasAnnotationItemController.toItemPos = staticmethod(KadasAnnotationItemController.toItemPos)
    KadasAnnotationItemController.toItemRect = staticmethod(KadasAnnotationItemController.toItemRect)
    KadasAnnotationItemController.toMapRect = staticmethod(KadasAnnotationItemController.toMapRect)
    KadasAnnotationItemController.pickTolSqr = staticmethod(KadasAnnotationItemController.pickTolSqr)
    KadasAnnotationItemController.__virtual_methods__ = ['populateContextMenu', 'onDoubleClick', 'hitTest', 'intersects', 'applyPersistedStyle', 'persistStyle', 'createStyleEditor', 'generateShadows', 'shadowIds', 'setShadowIds']
    KadasAnnotationItemController.__abstract_methods__ = ['itemType', 'itemName', 'createItem', 'nodes', 'startPart', 'setCurrentPoint', 'setCurrentAttributes', 'continuePart', 'endPart', 'drawAttribs', 'drawAttribsFromPosition', 'positionFromDrawAttribs', 'getEditContext', 'edit', 'editAttribsFromPosition', 'positionFromEditAttribs', 'position', 'setPosition', 'translate']
except (NameError, AttributeError):
    pass
