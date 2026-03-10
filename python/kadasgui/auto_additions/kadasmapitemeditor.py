# The following has been generated automatically from kadas/gui/mapitemeditors/kadasmapitemeditor.h
# monkey patching scoped based enum
KadasMapItemEditor.CreateItemEditor = KadasMapItemEditor.EditorType.CreateItemEditor
KadasMapItemEditor.CreateItemEditor.is_monkey_patched = True
KadasMapItemEditor.EditorType.CreateItemEditor.__doc__ = ""
KadasMapItemEditor.EditItemEditor = KadasMapItemEditor.EditorType.EditItemEditor
KadasMapItemEditor.EditItemEditor.is_monkey_patched = True
KadasMapItemEditor.EditorType.EditItemEditor.__doc__ = ""
KadasMapItemEditor.EditorType.__doc__ = """

* ``CreateItemEditor``: 
* ``EditItemEditor``: 

"""
# --
try:
    KadasMapItemEditor.__virtual_methods__ = ['setItem', 'reset']
    KadasMapItemEditor.__abstract_methods__ = ['syncItemToWidget', 'syncWidgetToItem']
except (NameError, AttributeError):
    pass
