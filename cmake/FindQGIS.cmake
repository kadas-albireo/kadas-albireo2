# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file COPYING-CMAKE-SCRIPTS or https://cmake.org/licensing for details.

#[=======================================================================[.rst:
FindQGIS
---------

CMake module to search for QGIS library

IMPORTED targets
^^^^^^^^^^^^^^^^
This module defines the following :prop_tgt:`IMPORTED` target:

 - ``QGIS::Core``
 - ``QGIS::Gui``
 - ``QGIS::Analysis``

Variables
^^^^^^^^^
This module defines the following variables:

``QGIS_FOUND``
  if the library is found

``QGIS_LIBRARIES``
  full path to the libraries

``QGIS_INCLUDE_DIRS``
  where to find the library headers

``QGIS_PREFIX_PATH``
  the base path of the qgis installation

``QGIS_VERSION_STRING``
  version string of QGIS

#]=======================================================================]

find_path(QGIS_INCLUDE_DIR qgis/qgis.h
          PATHS
            ${QGIS_ROOT}/include/
          DOC "Path to QGIS include directory")

include(SelectLibraryConfigurations)
macro(_find_qgis_library _lib_name _component)
  if(NOT QGIS_${_component}_LIBRARY)
    find_library(QGIS_${_component}_LIBRARY_RELEASE NAMES qgis_${_lib_name})
    find_library(QGIS_${_component}_LIBRARY_DEBUG NAMES qgis_${_lib_name})
    select_library_configurations(QGIS_${_component})
    mark_as_advanced(QGIS_${_component}_LIBRARY_RELEASE QGIS_${_component}_LIBRARY_DEBUG)
  endif()

  if(QGIS_${_component}_LIBRARY)
    list(APPEND QGIS_LIBRARIES QGIS_${_component}_LIBRARY)
    if(NOT TARGET QGIS::${_component})
      add_library(QGIS::${_component} UNKNOWN IMPORTED)
      set_target_properties(QGIS::${_component} PROPERTIES
                            IMPORTED_LINK_INTERFACE_LANGUAGES "CXX")
      target_include_directories(QGIS::${_component} INTERFACE
                                "${QGIS_INCLUDE_DIR}"
                                "${QGIS_INCLUDE_DIR}/qgis" # Required for includes in qgis .sip files
                                )
      if(EXISTS "${QGIS_${_component}_LIBRARY}")
        set_target_properties(QGIS::${_component} PROPERTIES
          IMPORTED_LINK_INTERFACE_LANGUAGES "CXX"
          IMPORTED_LOCATION "${QGIS_${_component}_LIBRARY}")
      endif()
                          #      if(EXISTS "${QGIS_${_component}_LIBRARY_RELEASE}")
                          #        set_property(TARGET QGIS::${_component} APPEND PROPERTY
                          #          IMPORTED_CONFIGURATIONS RELEASE)
                          #        set_target_properties(QGIS::${_component} PROPERTIES
                          #          IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "CXX"
                          #          IMPORTED_LOCATION_RELEASE "${QGIS_${_component}_LIBRARY_RELEASE}")
                          #      endif()
                          #      if(EXISTS "${QGIS_${_component}_LIBRARY_DEBUG}")
                          #        set_property(TARGET QGIS::${_component} APPEND PROPERTY
                          #          IMPORTED_CONFIGURATIONS DEBUG)
                          #        set_target_properties(QGIS::${_component} PROPERTIES
                          #          IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "CXX"
                          #          IMPORTED_LOCATION_DEBUG "${QGIS_${_component}_LIBRARY_DEBUG}")
                          #      endif()
    endif()
  endif()
  mark_as_advanced(QGIS_${_component}_LIBRARY)
endmacro()

if(Core IN_LIST QGIS_FIND_COMPONENTS)
  _find_qgis_library(core Core)
endif()
if(Analysis IN_LIST QGIS_FIND_COMPONENTS)
  _find_qgis_library(analysis Analysis)
  target_link_libraries(QGIS::Analysis INTERFACE QGIS::Core)
endif()
if(Gui IN_LIST QGIS_FIND_COMPONENTS)
  _find_qgis_library(gui Gui)
  target_link_libraries(QGIS::Gui INTERFACE QGIS::Core)
  if (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
      _find_qgis_library(native Native)
      target_link_libraries(QGIS::Core INTERFACE QGIS::Native)
  endif()
endif()


if(QGIS_INCLUDE_DIR)
  set(_qgsconfig_h "${QGIS_INCLUDE_DIR}/qgis/qgsconfig.h")
  file(STRINGS "${_qgsconfig_h}" _qgsversion_str REGEX "^#define VERSION .*$")
  string(REGEX REPLACE "^#define VERSION +\"([0-9]+\\.[0-9]+\\.[0-9]+).*$" "\\1" QGIS_VERSION_STRING "${_qgsversion_str}")
endif ()

foreach(_component ${QGIS_FIND_COMPONENTS})
  if(${_component} STREQUAL "Python")
    set(QGIS_PYTHON_MODULE_DIR "" CACHE PATH "Path to QGIS Python Modules")
    if(QGIS_PYTHON_MODULE_DIR STREQUAL "")
      set(CMD ${Python_EXECUTABLE} -c "import os;import qgis;print(os.path.dirname(qgis.__file__))")
      execute_process(COMMAND ${CMD} OUTPUT_VARIABLE QGIS_PYTHON_MODULE_DIR COMMAND_ERROR_IS_FATAL ANY ECHO_ERROR_VARIABLE OUTPUT_STRIP_TRAILING_WHITESPACE)
    endif()
    set(QGIS_SIP_DIR ${QGIS_PYTHON_MODULE_DIR}/bindings)
    message(STATUS "QGIS Python Module Dir: ${QGIS_PYTHON_MODULE_DIR}")

    # Add a cmake target: PYTHON_MODULE_DIR is not cmake native
    add_library(QGIS::Python UNKNOWN IMPORTED)
    set_target_properties(QGIS::Python PROPERTIES PYTHON_MODULE_DIR ${QGIS_PYTHON_MODULE_DIR})
  else()
    if(QGIS_FIND_REQUIRED_${_component})
      list(APPEND _required_libs "QGIS_${_component}_LIBRARY")
    endif()
  endif()
endforeach()
unset(_component)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(QGIS
                                  REQUIRED_VARS ${_required_libs} QGIS_INCLUDE_DIR
                                  VERSION_VAR QGIS_VERSION_STRING)
mark_as_advanced(QGIS_INCLUDE_DIR)
get_filename_component(_qgis_lib_path "${QGIS_Core_LIBRARY}" DIRECTORY)
get_filename_component(QGIS_PREFIX_PATH "${_qgis_lib_path}/.." ABSOLUTE)
unset(_qgis_lib_path)

if(QGIS_FOUND)
  set(QGIS_INCLUDE_DIRS ${QGIS_INCLUDE_DIR})
endif()

if(WITH_VCPKG)
  include("cmake/qgis-cmake-wrapper.cmake")
else()
  target_include_directories(QGIS::Core INTERFACE ${GDAL_INCLUDE_DIR})
  find_package(Qca-qt5 REQUIRED)
  target_link_libraries(QGIS::Core INTERFACE qca-qt5)
  find_package(Qt5Keychain REQUIRED)
  target_include_directories(QGIS::Core INTERFACE ${QTKEYCHAIN_INCLUDE_DIR})
  find_package(Qt5SerialPort REQUIRED)
  target_link_libraries(QGIS::Core INTERFACE Qt5::SerialPort)
endif()
