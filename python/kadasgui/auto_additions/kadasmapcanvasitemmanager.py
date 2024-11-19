# The following has been generated automatically from kadas/gui/kadasmapcanvasitemmanager.h
try:
    KadasMapCanvasItemManager.instance = staticmethod(KadasMapCanvasItemManager.instance)
    KadasMapCanvasItemManager.addItem = staticmethod(KadasMapCanvasItemManager.addItem)
    KadasMapCanvasItemManager.removeItem = staticmethod(KadasMapCanvasItemManager.removeItem)
    KadasMapCanvasItemManager.removeItemAfterRefresh = staticmethod(KadasMapCanvasItemManager.removeItemAfterRefresh)
    KadasMapCanvasItemManager.clear = staticmethod(KadasMapCanvasItemManager.clear)
    KadasMapCanvasItemManager.__signal_arguments__ = {'itemAdded': ['item: KadasMapItem'], 'itemWillBeRemoved': ['item: KadasMapItem']}
except ValueError:
    pass
