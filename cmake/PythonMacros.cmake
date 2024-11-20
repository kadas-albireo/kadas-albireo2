# Python macros
# ~~~
# Copyright (c) 2007, Simon Edwards <simon@simonzone.com>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#
# This file defines the following macros:
#
# PYTHON_INSTALL (SOURCE_FILE DESTINATION_DIR)
#     Install the SOURCE_FILE, which is a Python .py file, into the
#     destination directory during install. The file will be byte compiled
#     and both the .py file and .pyc file will be installed.

get_filename_component(PYTHON_MACROS_MODULE_PATH ${CMAKE_CURRENT_LIST_FILE}
                       PATH)

macro(PYTHON_INSTALL SOURCE_FILE DESTINATION_DIR)

  find_file(_python_compile_py PythonCompile.py PATHS ${CMAKE_MODULE_PATH})

  # Install the source file.
  install(FILES ${SOURCE_FILE} DESTINATION ${DESTINATION_DIR})

  # Byte compile and install the .pyc file.
  get_filename_component(_absfilename ${SOURCE_FILE} ABSOLUTE)
  get_filename_component(_filename ${SOURCE_FILE} NAME)
  get_filename_component(_filenamebase ${SOURCE_FILE} NAME_WE)
  get_filename_component(_basepath ${SOURCE_FILE} PATH)

  if(WIN32)
    string(REGEX REPLACE ".:/" "/" _basepath "${_basepath}")
  endif(WIN32)

  set(_bin_py ${CMAKE_CURRENT_BINARY_DIR}/${_basepath}/${_filename})
  set(_bin_pyc ${CMAKE_CURRENT_BINARY_DIR}/${_basepath}/${_filenamebase}.pyc)

  file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${_basepath})

  set(_message "-DMESSAGE=Byte-compiling ${_bin_py}")

  get_filename_component(_abs_bin_py ${_bin_py} ABSOLUTE)
  if(_abs_bin_py STREQUAL ${_absfilename}) # Don't copy the file onto itself.
    add_custom_command(
      TARGET compile_python_files
      COMMAND ${CMAKE_COMMAND} -E echo ${message}
      COMMAND ${Python_EXECUTABLE} ${_python_compile_py} ${_bin_py} DEPENDS
              ${_absfilename})
  else(_abs_bin_py STREQUAL ${_absfilename})
    add_custom_command(
      TARGET compile_python_files
      COMMAND ${CMAKE_COMMAND} -E echo ${message}
      COMMAND ${CMAKE_COMMAND} -E copy ${_absfilename} ${_bin_py}
      COMMAND ${Python_EXECUTABLE} ${_python_compile_py} ${_bin_py} DEPENDS
              ${_absfilename})
  endif(_abs_bin_py STREQUAL ${_absfilename})

  install(FILES ${_bin_pyc} DESTINATION ${DESTINATION_DIR})
endmacro(PYTHON_INSTALL)
