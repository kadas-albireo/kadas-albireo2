###################################################
# osgEarth plugin
###################################################

SET(OSGEARTH_PLUGIN_PREFIX "")
SET(LIB_POSTFIX ${LIB_SUFFIX})
SET(DYNAMIC_OSGEARTH "ON" )
SET(OSGEARTH_USER_DEFINED_DYNAMIC_OR_STATIC "SHARED")

# SET(CMAKE_SHARED_MODULE_PREFIX ${OSGEARTH_PLUGIN_PREFIX})
SET(TARGET_DEFAULT_PREFIX "osgdb_")
SET(TARGET_DEFAULT_LABEL_PREFIX "Plugin")

INCLUDE( OsgEarthMacroUtils )

SET(TARGET_SRC
    kadasglobefeaturesource.cpp
)

SET (TARGET_MOC_HDRS
    kadasglobefeaturesource.h
)

SET(TARGET_HDRS
    kadasglobefeaturecursor.h
    kadasglobefeatureoptions.h
    kadasglobefeatureutils.h
)

QT5_WRAP_CPP(TARGET_MOC_SRCS  ${TARGET_MOC_HDRS})

SET(TARGET_SRC ${TARGET_SRC} ${TARGET_MOC_SRCS})

SET(TARGET_COMMON_LIBRARIES ${TARGET_COMMON_LIBRARIES}
  ${QGIS_CORE_LIBRARY}
  ${QGIS_GUI_LIBRARY}
  ${OSGDB_LIBRARY}
  ${OSG_LIBRARY}
  ${OSGEARTH_LIBRARY}
  ${OSGEARTHFEATURES_LIBRARY}
  ${OSGEARTHSYMBOLOGY_LIBRARY}
  ${OSGEARTHUTIL_LIBRARY}
  ${OPENTHREADS_LIBRARY}
  Qt5::Widgets
  Qt5::Xml
)
SETUP_PLUGIN(osgearth_feature_qgis)

# to install public driver includes:
SET(LIB_NAME feature_qgis)
SET(LIB_PUBLIC_HEADERS ${TARGET_HDRS} ${TARGET_MOC_HDRS})
INCLUDE(ModuleInstallOsgEarthDriverIncludes OPTIONAL)
