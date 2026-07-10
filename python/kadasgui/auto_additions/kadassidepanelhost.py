# The following has been generated automatically from kadas/gui/kadassidepanelhost.h
# monkey patching scoped based enum
KadasSidePanelHost.Edge.Left.__doc__ = ""
KadasSidePanelHost.Edge.Right.__doc__ = ""
KadasSidePanelHost.Edge.__doc__ = """

* ``Left``: 
* ``Right``: 

"""
# --
try:
    KadasSidePanelHost.objectNameForEdge = staticmethod(KadasSidePanelHost.objectNameForEdge)
except (NameError, AttributeError):
    pass
