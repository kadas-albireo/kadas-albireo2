# The following has been generated automatically from kadas/gui/kadasfeaturepicker.h
# monkey patching scoped based enum
KadasFeaturePicker.PICK_OBJECTIVE_ANY = KadasFeaturePicker.PickObjective.PICK_OBJECTIVE_ANY
KadasFeaturePicker.PICK_OBJECTIVE_ANY.is_monkey_patched = True
KadasFeaturePicker.PickObjective.PICK_OBJECTIVE_ANY.__doc__ = ""
KadasFeaturePicker.PICK_OBJECTIVE_TOOLTIP = KadasFeaturePicker.PickObjective.PICK_OBJECTIVE_TOOLTIP
KadasFeaturePicker.PICK_OBJECTIVE_TOOLTIP.is_monkey_patched = True
KadasFeaturePicker.PickObjective.PICK_OBJECTIVE_TOOLTIP.__doc__ = ""
KadasFeaturePicker.PickObjective.__doc__ = """

* ``PICK_OBJECTIVE_ANY``: 
* ``PICK_OBJECTIVE_TOOLTIP``: 

"""
# --
try:
    KadasFeaturePicker.PickResult.__attribute_docs__ = {'annotationLayer': 'When non-null, the pick hit a :py:class:`QgsAnnotationItem` in this layer.', 'annotationItemId': 'Identifier of the picked annotation item within ``annotationLayer``.'}
    KadasFeaturePicker.PickResult.__annotations__ = {'annotationLayer': 'QgsAnnotationLayer', 'annotationItemId': str}
except (NameError, AttributeError):
    pass
try:
    KadasFeaturePicker.pick = staticmethod(KadasFeaturePicker.pick)
except (NameError, AttributeError):
    pass
