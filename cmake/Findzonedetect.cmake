# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

#[=======================================================================[.rst:
FindZonedetect
--------------

Finds the Zonedetect library.

Imported Targets
^^^^^^^^^^^^^^^^

This module provides the following imported targets, if found:

``zonedetect::ZoneDetect``
  The Zonedetect library

Result Variables
^^^^^^^^^^^^^^^^

This will define the following variables:

``zonedetect_FOUND``
  True if the system has the Zonedetect library.

#]=======================================================================]

if(TARGET zonedetect::ZoneDetect)
  return()
endif()
# Try to find vcpkg version (shamelessly ignoring any input hints)
find_package(unofficial-zonedetect CONFIG QUIET)
if(TARGET unofficial::zonedetect::ZoneDetect)
  add_library(zonedetect::ZoneDetect ALIAS unofficial::zonedetect::ZoneDetect)
  set(zonedetect_FOUND TRUE)
else()
  find_library(zonedetect_LIBRARY NAMES zonedetect)
  find_path(zonedetect_INCLUDE_DIR NAMES zonedetect.h)
  if(zonedetect_INCLUDE_DIR AND zonedetect_LIBRARY)
    add_library(zonedetect::ZoneDetect UNKNOWN IMPORTED)
    target_link_libraries(zonedetect::ZoneDetect
                          INTERFACE ${zonedetect_LIBRARY})
    target_include_directories(zonedetect::ZoneDetect
                               INTERFACE ${zonedetect_INCLUDE_DIR})
    set_target_properties(zonedetect::ZoneDetect
                          PROPERTIES IMPORTED_LOCATION ${zonedetect_LIBRARY})
    set(zonedetect_FOUND TRUE)
  endif()
endif()
