# The following has been generated automatically from kadas/core/kadasstatehistory.h
# monkey patching scoped based enum
KadasStateHistory.ChangeType.Undo.__doc__ = ""
KadasStateHistory.ChangeType.Redo.__doc__ = ""
KadasStateHistory.ChangeType.__doc__ = """

* ``Undo``: 
* ``Redo``: 

"""
# --
try:
    KadasStateHistory.__signal_arguments__ = {'canUndoChanged': [': bool'], 'canRedoChanged': [': bool'], 'stateChanged': ['changeType: KadasStateHistory.ChangeType', 'state: KadasStateHistory.State', 'prevState: KadasStateHistory.State']}
except AttributeError:
    pass
