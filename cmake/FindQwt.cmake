# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

#[=======================================================================[.rst:
FindQwt
-------

Finds the Qwt library.

Imported Targets
^^^^^^^^^^^^^^^^

This module provides the following imported targets, if found:

``qwt::Qwt``
  The Qwt library

Result Variables
^^^^^^^^^^^^^^^^

This will define the following variables:

``Qwt_FOUND``
  True if the system has the Qwt library.

#]=======================================================================]

if(TARGET Qwt::Qwt)
  return()
endif()
find_package(unofficial-qwt CONFIG)
if(TARGET unofficial::qwt::qwt) # vcpkg version
  add_library(Qwt::Qwt ALIAS unofficial::qwt::qwt)
  set(Qwt_FOUND TRUE)
else() # pkgconfig
  find_package(PkgConfig REQUIRED)
  pkg_search_module(PC_Qwt IMPORTED_TARGET Qt5Qwt6)
  if(PC_Qwt_FOUND)
    add_library(Qwt::Qwt ALIAS PkgConfig::PC_Qwt)
    set(Qwt_FOUND TRUE)
  endif()
endif()
