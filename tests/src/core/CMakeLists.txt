# ##############################################################################
# Don't forget to include output directory, otherwise the UI file won't be
# wrapped!
include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

include_directories(SYSTEM ${QT_INCLUDE_DIR})

# ##############################################################################
# Tests:

set(TESTS testkadas.cpp)

foreach(TESTSRC ${TESTS})
  add_kadas_test(${TESTSRC} MODULE core LINKEDLIBRARIES qgis_core)
endforeach(TESTSRC)
