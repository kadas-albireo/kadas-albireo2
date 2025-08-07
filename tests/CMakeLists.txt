if(ENABLE_TESTS)
  # create a variable to specify where our test data is so that unit tests can
  # use TEST_DATA_DIR to locate the test data. See CMakeLists in test dirs for
  # more info
  set(TEST_DATA_DIR "${CMAKE_CURRENT_SOURCE_DIR}/testdata")

  add_subdirectory(src)
endif()
