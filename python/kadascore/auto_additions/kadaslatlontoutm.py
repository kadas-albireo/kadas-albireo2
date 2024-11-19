# The following has been generated automatically from kadas/core/kadaslatlontoutm.h
# monkey patching scoped based enum
KadasLatLonToUTM.Level.Major.__doc__ = ""
KadasLatLonToUTM.Level.Minor.__doc__ = ""
KadasLatLonToUTM.Level.OnlyLabels.__doc__ = ""
KadasLatLonToUTM.Level.__doc__ = """

* ``Major``: 
* ``Minor``: 
* ``OnlyLabels``: 

"""
# --
# monkey patching scoped based enum
KadasLatLonToUTM.GridUTM = KadasLatLonToUTM.GridMode.GridUTM
KadasLatLonToUTM.GridUTM.is_monkey_patched = True
KadasLatLonToUTM.GridMode.GridUTM.__doc__ = ""
KadasLatLonToUTM.GridMGRS = KadasLatLonToUTM.GridMode.GridMGRS
KadasLatLonToUTM.GridMGRS.is_monkey_patched = True
KadasLatLonToUTM.GridMode.GridMGRS.__doc__ = ""
KadasLatLonToUTM.GridMode.__doc__ = """

* ``GridUTM``: 
* ``GridMGRS``: 

"""
# --
try:
    KadasLatLonToUTM.UTM2LL = staticmethod(KadasLatLonToUTM.UTM2LL)
    KadasLatLonToUTM.LL2UTM = staticmethod(KadasLatLonToUTM.LL2UTM)
    KadasLatLonToUTM.UTM2MGRS = staticmethod(KadasLatLonToUTM.UTM2MGRS)
    KadasLatLonToUTM.MGRS2UTM = staticmethod(KadasLatLonToUTM.MGRS2UTM)
    KadasLatLonToUTM.zoneNumber = staticmethod(KadasLatLonToUTM.zoneNumber)
    KadasLatLonToUTM.hemisphereLetter = staticmethod(KadasLatLonToUTM.hemisphereLetter)
    KadasLatLonToUTM.zoneName = staticmethod(KadasLatLonToUTM.zoneName)
    KadasLatLonToUTM.computeGrid = staticmethod(KadasLatLonToUTM.computeGrid)
except ValueError:
    pass
