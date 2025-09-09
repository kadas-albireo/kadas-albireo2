
set(KADAS_TEST_HDRS kadastest.h)

add_library(kadas_test ${LIBRARY_TYPE} ${KADAS_TEST_HDRS})

target_link_libraries(kadas)
