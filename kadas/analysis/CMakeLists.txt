FILE(GLOB kadas_analysis_SRC *.cpp)
LIST(SORT kadas_analysis_SRC)

FILE(GLOB kadas_analysis_HDR *.h)
LIST(SORT kadas_analysis_HDR)

ADD_LIBRARY(kadas_analysis
    ${kadas_analysis_SRC}
    ${kadas_analysis_HDR}
)

TARGET_LINK_LIBRARIES(kadas_analysis
  Qt5::Widgets
  Qt5::Network
  Qt5::Xml
  kadas_core
)

target_include_directories(kadas_analysis PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

GENERATE_EXPORT_HEADER(
  kadas_analysis
  BASE_NAME KADAS_ANALYSIS
  EXPORT_FILE_NAME kadas_analysis.h
)
SET(kadas_analysis_HDR ${kadas_analysis_HDR} ${CMAKE_CURRENT_BINARY_DIR}/kadas_analysis.h)

INSTALL(TARGETS kadas_analysis
  RUNTIME DESTINATION bin COMPONENT libraries
  ARCHIVE DESTINATION lib${LIB_SUFFIX} COMPONENT libraries
  LIBRARY DESTINATION lib${LIB_SUFFIX} COMPONENT libraries)

INSTALL(FILES ${kadas_analysis_HDR} DESTINATION include/kadas/analysis)
