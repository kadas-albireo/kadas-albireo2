file(GLOB kadas_analysis_SRC *.cpp)
list(SORT kadas_analysis_SRC)

file(GLOB kadas_analysis_HDR *.h)
list(SORT kadas_analysis_HDR)

add_library(kadas_analysis ${kadas_analysis_SRC} ${kadas_analysis_HDR})

target_link_libraries(
  kadas_analysis Qt6::Widgets Qt6::Network Qt6::Xml kadas_core
)

target_include_directories(kadas_analysis PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

generate_export_header(
  kadas_analysis BASE_NAME KADAS_ANALYSIS EXPORT_FILE_NAME kadas_analysis.h
)
set(kadas_analysis_HDR ${kadas_analysis_HDR}
                       ${CMAKE_CURRENT_BINARY_DIR}/kadas_analysis.h
)

install(
  TARGETS kadas_analysis
  RUNTIME DESTINATION ${KADAS_BIN_SUBDIR}
  ARCHIVE DESTINATION ${KADAS_LIB_SUBDIR}
  LIBRARY DESTINATION ${KADAS_LIB_SUBDIR}
)

# Headers are not part of the macOS app bundle
if(NOT KADAS_MAC_BUNDLE)
  install(FILES ${kadas_analysis_HDR} DESTINATION include/kadas/analysis)
endif()
