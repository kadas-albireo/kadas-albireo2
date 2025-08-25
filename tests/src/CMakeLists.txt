if(ENABLE_TESTS)

  macro(add_kadas_test TESTSRC)
    set(options OPTIONAL)
    set(oneValueArgs MODULE LABELS)
    set(multiValueArgs LINKEDLIBRARIES DEPENDENCIES)
    cmake_parse_arguments(
      ARG_QGIS_TEST "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN}
    )

    set(TESTNAME ${TESTSRC})
    string(REPLACE "test" "" TESTNAME ${TESTNAME})
    string(REPLACE "qgs" "" TESTNAME ${TESTNAME})
    string(REPLACE ".cpp" "" TESTNAME ${TESTNAME})
    set(TESTNAME "test_${ARG_QGIS_TEST_MODULE}_${TESTNAME}")
    add_executable(${TESTNAME} ${TESTSRC} ${util_SRCS})
    target_compile_features(${TESTNAME} PRIVATE cxx_std_17)
    set_target_properties(${TESTNAME} PROPERTIES AUTORCC TRUE)
    target_link_libraries(${TESTNAME} ${ARG_QGIS_TEST_LINKEDLIBRARIES})
    target_link_libraries(${TESTNAME} kadas_test)
    add_test(${TESTNAME} ${CMAKE_BINARY_DIR}/output/bin/${TESTNAME}
             -maxwarnings 10000
    )
    if(DEFINED ARG_QGIS_TEST_DEPENDENCIES)
      add_dependencies(${TESTNAME} ${ARG_QGIS_TEST_DEPENDENCIES})
    endif()
    if(DEFINED ARG_QGIS_TEST_LABELS)
      set_tests_properties(
        ${TESTNAME} PROPERTIES LABELS ${ARG_QGIS_TEST_LABELS}
      )
    endif()

    set_tests_properties(${TESTNAME} PROPERTIES FIXTURES_REQUIRED SOURCETREE)
    target_compile_definitions(
      ${TESTNAME} PRIVATE "CMAKE_SOURCE_DIR=\"${CMAKE_SOURCE_DIR}\""
    )
  endmacro()

  # ############################################################################
  # Compiler defines

  # This define is used for tests that need to locate the test data under
  # tests/testdata in the kadas source tree. the TEST_DATA_DIR variable is set
  # in the top level CMakeLists.txt
  add_definitions(-DTEST_DATA_DIR="${TEST_DATA_DIR}")

  add_subdirectory(core)

endif()
