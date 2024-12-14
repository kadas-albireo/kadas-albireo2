# Find PyQt5
# ~~~
# Copyright (c) 2007-2008, Simon Edwards <simon@simonzone.com>
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#
# PyQt5 website: http://www.riverbankcomputing.co.uk/pyqt/index.php
#
# Find the installed version of PyQt5. FindPyQt5 should only be called after
# Python has been found.
#
# This file defines the following variables:
#
# PYQT5_VERSION_STR - The version of PyQt5 as a human readable string.
#
# PYQT5_SIP_DIR - The directory holding the PyQt5 .sip files.
#
# PYQT5_SIP_FLAGS - The SIP flags used to build PyQt.

if(EXISTS PYQT5_VERSION_STR)
  # Already in cache, be silent
  set(PYQT5_FOUND TRUE)
else(EXISTS PYQT5_VERSION_STR)

  if(SIP_BUILD_EXECUTABLE)
    # SIP >= 5.0 path

    file(GLOB _pyqt5_metadata "${Python_SITEARCH}/PyQt5-5*.dist-info/METADATA")
    file(READ ${_pyqt5_metadata} _pyqt5_metadata_contents)
    string(REGEX REPLACE ".*\nVersion: ([^\n]+).*$" "\\1" PYQT5_VERSION_STR
                         ${_pyqt5_metadata_contents}
    )

    if(PYQT5_VERSION_STR)
      set(PYQT5_MOD_DIR "${Python_SITEARCH}/PyQt5")
      set(PYQT5_SIP_DIR "${Python_SITEARCH}/PyQt5/bindings")

      set(PYQT5_FOUND TRUE)
    endif(PYQT5_VERSION_STR)

  else(SIP_BUILD_EXECUTABLE)
    # SIP 4.x path

    find_file(
      _find_pyqt5_py FindPyQt5.py
      PATHS ${CMAKE_MODULE_PATH}
      NO_CMAKE_FIND_ROOT_PATH
    )

    execute_process(
      COMMAND ${Python_EXECUTABLE} ${_find_pyqt5_py}
      OUTPUT_VARIABLE pyqt_config
    )
    if(pyqt_config)
      string(REGEX REPLACE "^pyqt_version_str:([^\n]+).*$" "\\1"
                           PYQT5_VERSION_STR ${pyqt_config}
      )
      string(REGEX REPLACE ".*\npyqt_mod_dir:([^\n]+).*$" "\\1" PYQT5_MOD_DIR
                           ${pyqt_config}
      )
      string(REGEX REPLACE ".*\npyqt_sip_dir:([^\n]+).*$" "\\1" PYQT5_SIP_DIR
                           ${pyqt_config}
      )
      if(EXISTS ${PYQT5_SIP_DIR}/Qt5)
        set(PYQT5_SIP_DIR ${PYQT5_SIP_DIR}/Qt5)
      endif(EXISTS ${PYQT5_SIP_DIR}/Qt5)
      string(REGEX REPLACE ".*\npyqt_sip_flags:([^\n]+).*$" "\\1"
                           PYQT5_SIP_FLAGS ${pyqt_config}
      )
      string(REGEX REPLACE ".*\npyqt_sip_module:([^\n]+).*$" "\\1"
                           PYQT5_SIP_IMPORT ${pyqt_config}
      )
      set(PYQT5_FOUND TRUE)
    endif(pyqt_config)

  endif(SIP_BUILD_EXECUTABLE)

  if(PYQT5_FOUND)
    if(NOT PyQt5_FIND_QUIETLY)
      message(STATUS "Found PyQt5 version: ${PYQT5_VERSION_STR}")
    endif(NOT PyQt5_FIND_QUIETLY)
  else(PYQT5_FOUND)
    if(PyQt5_FIND_REQUIRED)
      message(FATAL_ERROR "Could not find PyQt5")
    endif(PyQt5_FIND_REQUIRED)
  endif(PYQT5_FOUND)

endif(EXISTS PYQT5_VERSION_STR)
