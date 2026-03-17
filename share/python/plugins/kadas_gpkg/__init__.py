# -*- coding: utf-8 -*-


def classFactory(iface):
    from .kadas_gpkg import KadasGpkg

    return KadasGpkg(iface)
