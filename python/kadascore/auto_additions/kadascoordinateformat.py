# The following has been generated automatically from kadas/core/kadascoordinateformat.h
# monkey patching scoped based enum
KadasCoordinateFormat.Default = KadasCoordinateFormat.Format.Default
KadasCoordinateFormat.Default.is_monkey_patched = True
KadasCoordinateFormat.Format.Default.__doc__ = ""
KadasCoordinateFormat.DegMinSec = KadasCoordinateFormat.Format.DegMinSec
KadasCoordinateFormat.DegMinSec.is_monkey_patched = True
KadasCoordinateFormat.Format.DegMinSec.__doc__ = ""
KadasCoordinateFormat.DegMin = KadasCoordinateFormat.Format.DegMin
KadasCoordinateFormat.DegMin.is_monkey_patched = True
KadasCoordinateFormat.Format.DegMin.__doc__ = ""
KadasCoordinateFormat.DecDeg = KadasCoordinateFormat.Format.DecDeg
KadasCoordinateFormat.DecDeg.is_monkey_patched = True
KadasCoordinateFormat.Format.DecDeg.__doc__ = ""
KadasCoordinateFormat.UTM = KadasCoordinateFormat.Format.UTM
KadasCoordinateFormat.UTM.is_monkey_patched = True
KadasCoordinateFormat.Format.UTM.__doc__ = ""
KadasCoordinateFormat.MGRS = KadasCoordinateFormat.Format.MGRS
KadasCoordinateFormat.MGRS.is_monkey_patched = True
KadasCoordinateFormat.Format.MGRS.__doc__ = ""
KadasCoordinateFormat.Format.__doc__ = """

* ``Default``: 
* ``DegMinSec``: 
* ``DegMin``: 
* ``DecDeg``: 
* ``UTM``: 
* ``MGRS``: 

"""
# --
KadasCoordinateFormat.Format.baseClass = KadasCoordinateFormat
try:
    KadasCoordinateFormat.instance = staticmethod(KadasCoordinateFormat.instance)
    KadasCoordinateFormat.__signal_arguments__ = {'coordinateDisplayFormatChanged': ['format: KadasCoordinateFormat.Format', 'epsg: str'], 'heightDisplayUnitChanged': ['heightUnit: Qgis.DistanceUnit']}
except NameError:
    pass
