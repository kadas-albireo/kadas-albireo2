# Find PyQt6
# ~~~
# Copyright (c) 2007-2008, Simon Edwards <simon@simonzone.com>
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#
# PyQt6 website: http://www.riverbankcomputing.co.uk/pyqt/index.php
#
# Find the installed version of PyQt6. FindPyQt6 should only be called after
# Python has been found.
#
# This file defines the following variables:
#
# PYQT6_VERSION_STR - The version of PyQt6 as a human readable string.
#
# PYQT6_SIP_DIR - The directory holding the PyQt6 .sip files.
#
# PYQT6_SIP_FLAGS - The SIP flags used to build PyQt.

if(EXISTS PYQT6_VERSION_STR)
  # Already in cache, be silent
  set(PYQT6_FOUND TRUE)
else(EXISTS PYQT6_VERSION_STR)

  if(SIP_BUILD_EXECUTABLE)
    # SIP >= 5.0 path

    file(GLOB _pyqt6_metadata "${Python_SITEARCH}/PyQt6-*.dist-info/METADATA")
    if(_pyqt6_metadata)
      file(READ ${_pyqt6_metadata} _pyqt6_metadata_contents)
      string(REGEX REPLACE ".*\nVersion: ([^\n]+).*$" "\\1" PYQT6_VERSION_STR
                           ${_pyqt6_metadata_contents}
      )
    else(_pyqt6_metadata)
      execute_process(
        COMMAND
          ${Python_EXECUTABLE} -c
          "from PyQt6.QtCore import PYQT_VERSION_STR; print(PYQT_VERSION_STR, end='')"
        OUTPUT_VARIABLE PYQT6_VERSION_STR
      )
    endif(_pyqt6_metadata)

    if(PYQT6_VERSION_STR)
      execute_process(
        COMMAND
          ${Python_EXECUTABLE} -c
          "import os; import PyQt6; print(os.path.dirname(PyQt6.__file__), end='')"
        OUTPUT_VARIABLE PYQT6_MOD_DIR
      )
      set(PYQT6_SIP_DIR "${PYQT6_MOD_DIR}/bindings")
      find_program(__pyuic6 "pyuic6")
      get_filename_component(PYQT6_BIN_DIR ${__pyuic6} DIRECTORY)

      set(PYQT6_FOUND TRUE)
    endif(PYQT6_VERSION_STR)

  else(SIP_BUILD_EXECUTABLE)
    # SIP 4.x path

    find_file(
      _find_pyqt6_py FindPyQt6.py
      PATHS ${CMAKE_MODULE_PATH}
      NO_CMAKE_FIND_ROOT_PATH
    )

    execute_process(
      COMMAND ${Python_EXECUTABLE} ${_find_pyqt6_py}
      OUTPUT_VARIABLE pyqt_config
    )
    if(pyqt_config)
      string(REGEX REPLACE "^pyqt_version_str:([^\n]+).*$" "\\1"
                           PYQT6_VERSION_STR ${pyqt_config}
      )
      string(REGEX REPLACE ".*\npyqt_mod_dir:([^\n]+).*$" "\\1" PYQT6_MOD_DIR
                           ${pyqt_config}
      )
      string(REGEX REPLACE ".*\npyqt_sip_dir:([^\n]+).*$" "\\1" PYQT6_SIP_DIR
                           ${pyqt_config}
      )
      if(EXISTS ${PYQT6_SIP_DIR}/Qt6)
        set(PYQT6_SIP_DIR ${PYQT6_SIP_DIR}/Qt6)
      endif(EXISTS ${PYQT6_SIP_DIR}/Qt6)
      set(PYQT6_SIP_FLAGS "")
      string(REGEX REPLACE ".*\npyqt_bin_dir:([^\n]+).*$" "\\1" PYQT6_BIN_DIR
                           ${pyqt_config}
      )
      string(REGEX REPLACE ".*\npyqt_sip_module:([^\n]+).*$" "\\1"
                           PYQT6_SIP_IMPORT ${pyqt_config}
      )
      set(PYQT6_FOUND TRUE)
    endif(pyqt_config)

  endif(SIP_BUILD_EXECUTABLE)

  if(PYQT6_FOUND)
    if(NOT PyQt6_FIND_QUIETLY)
      message(STATUS "Found PyQt6 version: ${PYQT6_VERSION_STR}")
    endif(NOT PyQt6_FIND_QUIETLY)
  else(PYQT6_FOUND)
    if(PyQt6_FIND_REQUIRED)
      message(FATAL_ERROR "Could not find PyQt6")
    endif(PyQt6_FIND_REQUIRED)
  endif(PYQT6_FOUND)

endif(EXISTS PYQT6_VERSION_STR)
