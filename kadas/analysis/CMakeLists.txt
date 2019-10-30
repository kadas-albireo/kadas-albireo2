FIND_PACKAGE(OpenMP)
SET(CMAKE_AUTOMOC ON)

FILE(GLOB kadas_analysis_SRC *.cpp)
LIST(SORT kadas_analysis_SRC)

FILE(GLOB kadas_analysis_HDR *.h)
LIST(SORT kadas_analysis_HDR)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})
INCLUDE_DIRECTORIES(${GDAL_INCLUDE_DIR})

ADD_LIBRARY(kadas_analysis SHARED
    ${kadas_analysis_SRC}
    ${kadas_analysis_HDR}
)

TARGET_LINK_LIBRARIES(kadas_analysis
  Qt5::Widgets
  Qt5::Network
  Qt5::Xml
  ${QGIS_CORE_LIBRARY}
  ${QGIS_ANALYSIS_LIBRARY}
  ${GDAL_LIBRARIES}
  OpenMP::OpenMP_CXX
)

GENERATE_EXPORT_HEADER(
  kadas_analysis
  BASE_NAME KADAS_ANALYSIS
  EXPORT_FILE_NAME kadas_analysis.h
)
SET(kadas_analysis_HDR ${kadas_analysis_HDR} ${CMAKE_CURRENT_BINARY_DIR}/kadas_analysis.h)
