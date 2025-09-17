macro(add_kadas_test TESTSRC)
  set(options OPTIONAL)
  set(oneValueArgs MODULE)
  set(multiValueArgs LINKEDLIBRARIES DEPENDENCIES LABELS)
  cmake_parse_arguments(
    ARG_KADAS_TEST "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN}
  )

  set(TESTNAME ${TESTSRC})
  string(REPLACE "test" "" TESTNAME ${TESTNAME})
  string(REPLACE "qgs" "" TESTNAME ${TESTNAME})
  string(REPLACE ".cpp" "" TESTNAME ${TESTNAME})
  set(TESTNAME "test_${ARG_KADAS_TEST_MODULE}_${TESTNAME}")
  add_executable(${TESTNAME} ${TESTSRC} ${util_SRCS})
  set_target_properties(${TESTNAME} PROPERTIES AUTORCC TRUE)
  target_link_libraries(${TESTNAME} ${ARG_KADAS_TEST_LINKEDLIBRARIES})
  target_link_libraries(${TESTNAME} Qt5::Test)
  add_test(${TESTNAME} ${CMAKE_BINARY_DIR}/output/bin/${TESTNAME} -maxwarnings
           10000
  )
  if(DEFINED ARG_KADAS_TEST_DEPENDENCIES)
    add_dependencies(${TESTNAME} ${ARG_KADAS_TEST_DEPENDENCIES})
  endif()
  if(DEFINED ARG_KADAS_TEST_LABELS)
    set_tests_properties(${TESTNAME} PROPERTIES LABELS ${ARG_KADAS_TEST_LABELS})
  endif()

  set_tests_properties(${TESTNAME} PROPERTIES FIXTURES_REQUIRED SOURCETREE)
  target_compile_definitions(
    ${TESTNAME} PRIVATE "CMAKE_SOURCE_DIR=\"${CMAKE_SOURCE_DIR}\""
  )
  target_compile_definitions(
    ${TESTNAME} PRIVATE "TEST_DATA_DIR=\"${TEST_DATA_DIR}\""
  )
endmacro()

add_subdirectory(gui)
