add_definitions(-DTEST_DATA_DIR="${TEST_DATA_DIR}")

set(KADAS_TEST_HDRS kadastest.h)

add_library(kadas_test ${LIBRARY_TYPE} ${KADAS_TEST_HDRS})

# require c++17
target_compile_features(kadas_test PRIVATE cxx_std_17)

target_include_directories(
  kadas_test PUBLIC ${CMAKE_SOURCE_DIR}/kadas/test
                    ${CMAKE_BINARY_DIR}/kadas/test
)

target_link_libraries(kadas_test kadas_core Qt5::Test)
