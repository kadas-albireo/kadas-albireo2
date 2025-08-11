if(ENABLE_TESTS)
  # create a variable to specify where our test data is so that unit tests can
  # use TEST_DATA_DIR to locate the test data. See CMakeLists in test dirs for
  # more info
  set(TEST_DATA_DIR "${CMAKE_CURRENT_SOURCE_DIR}/testdata")

  # This define is used for tests that need to locate the test data under
  # tests/testdata in the kadas source tree. the TEST_DATA_DIR variable is set in
  # the top level CMakeLists.txt
  add_definitions(-DTEST_DATA_DIR="${TEST_DATA_DIR}")

  add_subdirectory(src)
endif()
