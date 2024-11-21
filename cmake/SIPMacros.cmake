# Macros for SIP
# ~~~
# Copyright (c) 2007, Simon Edwards <simon@simonzone.com>
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#
# SIP website: http://www.riverbankcomputing.co.uk/sip/index.php
#
# This file defines the following macros:
#
# ADD_SIP_PYTHON_MODULE (MODULE_NAME MODULE_SIP [library1, libaray2, ...])
#     Specifies a SIP file to be built into a Python module and installed.
#     MODULE_NAME is the name of Python module including any path name. (e.g.
#     os.sys, Foo.bar etc). MODULE_SIP the path and filename of the .sip file
#     to process and compile. libraryN are libraries that the Python module,
#     which is typically a shared library, should be linked to. The built
#     module will also be install into Python's site-packages directory.
#
# The behavior of the ADD_SIP_PYTHON_MODULE macro can be controlled by a
# number of variables:
#
# SIP_TAGS - List of tags to define when running SIP. (Corresponds to the -t
#     option for SIP.)
#
# SIP_CONCAT_PARTS - An integer which defines the number of parts the C++ code
#     of each module should be split into. Defaults to 8. (Corresponds to the
#     -j option for SIP.)
#
# SIP_DISABLE_FEATURES - List of feature names which should be disabled
#     running SIP. (Corresponds to the -x option for SIP.)
#
# SIP_EXTRA_OPTIONS - Extra command line options which should be passed on to
#     SIP.

set(SIP_TAGS)
set(SIP_CONCAT_PARTS 17)
set(SIP_DISABLE_FEATURES)
set(SIP_EXTRA_OPTIONS)
set(SIP_EXTRA_OBJECTS)

macro(GENERATE_SIP_PYTHON_MODULE_CODE MODULE_NAME MODULE_SIP SIP_FILES
      CPP_FILES)

  string(REPLACE "." "/" _x ${MODULE_NAME})
  get_filename_component(_parent_module_path ${_x} PATH)
  get_filename_component(_child_module_name ${_x} NAME)
  get_filename_component(_module_path ${MODULE_SIP} PATH)
  get_filename_component(_abs_module_sip ${MODULE_SIP} ABSOLUTE)

  # If this is not need anymore (using input configuration file for SIP files)
  # SIP could be run in the source rather than in binary directory
  set(_configured_module_sip
      ${CMAKE_CURRENT_BINARY_DIR}/${_module_path}/${_module_path}.sip)
  foreach(_sip_file ${SIP_FILES})
    get_filename_component(_sip_file_path ${_sip_file} PATH)
    get_filename_component(_sip_file_name_we ${_sip_file} NAME_WE)
    file(RELATIVE_PATH _sip_file_relpath ${CMAKE_CURRENT_SOURCE_DIR}
         "${_sip_file_path}/${_sip_file_name_we}")
    set(_out_sip_file "${CMAKE_CURRENT_BINARY_DIR}/${_sip_file_relpath}.sip")
    configure_file(${_sip_file} ${_out_sip_file})
  endforeach(_sip_file)

  set(_message "-DMESSAGE=Generating CPP code for module ${MODULE_NAME}")
  set(_sip_output_files)

  # Suppress warnings
  if(PEDANTIC)
    if(MSVC)
      add_definitions(
        /wd4189 # local variable is initialized but not referenced
        /wd4996 # deprecation warnings (bindings re-export deprecated methods)
        /wd4701 # potentially uninitialized variable used (sip generated code)
        /wd4702 # unreachable code (sip generated code)
        /wd4703 # potentially uninitialized local pointer variable 'sipType'
                # used
      )
    else(MSVC)
      # disable all warnings
      add_definitions(-w -Wno-deprecated-declarations)
      if(NOT APPLE)
        add_definitions(-fpermissive)
      endif(NOT APPLE)
    endif(MSVC)
  endif(PEDANTIC)

  if(MSVC)
    add_definitions(/bigobj)
  endif(MSVC)

  if(SIP_BUILD_EXECUTABLE)

    file(
      MAKE_DIRECTORY
      ${CMAKE_CURRENT_BINARY_DIR}/${_module_path}/build/${_child_module_name}
    )# Output goes in this dir.

    foreach(CONCAT_NUM RANGE 0 ${SIP_CONCAT_PARTS})
      if(${CONCAT_NUM} LESS ${SIP_CONCAT_PARTS})
        set(_sip_output_files
            ${_sip_output_files}
            ${CMAKE_CURRENT_BINARY_DIR}/${_module_path}/build/${_child_module_name}/sip${_child_module_name}part${CONCAT_NUM}.cpp
        )
      endif(${CONCAT_NUM} LESS ${SIP_CONCAT_PARTS})
    endforeach(CONCAT_NUM RANGE 0 ${SIP_CONCAT_PARTS})

    set(SIPCMD
        ${SIP_BUILD_EXECUTABLE}
        --no-make
        --concatenate=${SIP_CONCAT_PARTS}
        --qmake=${QMAKE_EXECUTABLE}
        --include-dir=${CMAKE_CURRENT_BINARY_DIR}
        --include-dir=${QGIS_SIP_DIR}
        --include-dir=${PYQT5_SIP_DIR})

    add_custom_command(
      OUTPUT ${_sip_output_files}
      COMMAND ${CMAKE_COMMAND} -E echo ${message}
      COMMAND ${SIPCMD}
      COMMAND ${CMAKE_COMMAND} -E touch ${_sip_output_files}
      WORKING_DIRECTORY ${_module_path}
      MAIN_DEPENDENCY ${_configured_module_sip}
      DEPENDS ${SIP_EXTRA_FILES_DEPEND}
      VERBATIM)

  else(SIP_BUILD_EXECUTABLE)
    message(FATAL_ERROR "sip-build (SIP_BUILD_EXECUTABLE) not found")
  endif(SIP_BUILD_EXECUTABLE)

  add_custom_target(generate_sip_${MODULE_NAME}_cpp_files
                    DEPENDS ${_sip_output_files})

  set(CPP_FILES ${sip_output_files})
endmacro(GENERATE_SIP_PYTHON_MODULE_CODE)

# Will compile and link the module
macro(BUILD_SIP_PYTHON_MODULE MODULE_NAME SIP_FILES EXTRA_OBJECTS)
  set(EXTRA_LINK_LIBRARIES ${ARGN})

  # We give this target a long logical target name. (This is to avoid having the
  # library name clash with any already install library names. If that happens
  # then cmake dependency tracking get confused.)
  string(REPLACE "." "_" _logical_name ${MODULE_NAME})
  set(_logical_name "python_module_${_logical_name}")
  get_filename_component(_module_path ${SIP_FILES} PATH)

  add_library(${_logical_name} MODULE ${_sip_output_files} ${EXTRA_OBJECTS})
  set_property(TARGET ${_logical_name} PROPERTY AUTOMOC OFF)
  target_include_directories(
    ${_logical_name} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/${_module_path}/build)
  if(NOT MSVC)
    target_compile_definitions(${_logical_name} PRIVATE protected=public)
  endif()

  if(${SIP_VERSION_STR} VERSION_LESS 5.0.0)
    # require c++14 only -- sip breaks with newer versions due to reliance on
    # throw(...) annotations removed in c++17
    target_compile_features(${_logical_name} PRIVATE cxx_std_14)
  endif(${SIP_VERSION_STR} VERSION_LESS 5.0.0)

  set_target_properties(${_logical_name} PROPERTIES CXX_VISIBILITY_PRESET
                                                    default)
  if(NOT APPLE)
    target_link_libraries(${_logical_name} ${Python_LIBRARIES})
  endif(NOT APPLE)
  target_link_libraries(${_logical_name} ${EXTRA_LINK_LIBRARIES})

  message(STATUS "${_logical_name} ${EXTRA_LINK_LIBRARIES}")
  if(APPLE)
    set_target_properties(${_logical_name}
                          PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
  endif(APPLE)
  set_target_properties(${_logical_name}
                        PROPERTIES PREFIX "" OUTPUT_NAME ${_child_module_name})

  target_link_libraries(${_logical_name} Python::Python)

  if(WIN32)
    set_target_properties(${_logical_name} PROPERTIES SUFFIX ".pyd")
  endif(WIN32)

  if(WIN32)
    get_target_property(_runtime_output ${_logical_name}
                        RUNTIME_OUTPUT_DIRECTORY)
    add_custom_command(
      TARGET ${_logical_name}
      POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E echo "Copying extension ${_child_module_name}"
      COMMAND
        ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:${_logical_name}>"
        "${_runtime_output}/${_child_module_name}.pyd" DEPENDS ${_logical_name})
  endif(WIN32)

  install(TARGETS ${_logical_name}
          DESTINATION "${SITEARCH_INSTALL_DIR}/${_parent_module_path}")
endmacro(
  BUILD_SIP_PYTHON_MODULE
  MODULE_NAME
  SIP_FILES
  EXTRA_OBJECTS)
