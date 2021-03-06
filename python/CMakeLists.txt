FIND_PACKAGE(PythonInterp 3 REQUIRED)
FIND_PACKAGE(PythonLibs REQUIRED)
EXECUTE_PROCESS(
  COMMAND "${PYTHON_EXECUTABLE}" -c "from distutils import sysconfig; print(sysconfig.get_python_lib(prefix='', plat_specific=True))"
  OUTPUT_VARIABLE PYTHON_SITE_PACKAGES_DIR
  OUTPUT_STRIP_TRAILING_WHITESPACE)

FIND_PACKAGE(SIP REQUIRED)
FIND_PACKAGE(PyQt5 REQUIRED)
INCLUDE(PythonMacros)
INCLUDE(PyQtMacros)
INCLUDE(SIPMacros)

SET(KADAS_PYTHON_OUTPUT_DIRECTORY ${KADAS_OUTPUT_DIRECTORY}/python/kadas)
SET(KADAS_PYTHON_INSTALL_DIRECTORY ${PYTHON_SITE_PACKAGES_DIR}/kadas)
SET (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${KADAS_PYTHON_OUTPUT_DIRECTORY})
SET (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${KADAS_PYTHON_OUTPUT_DIRECTORY})


INCLUDE_DIRECTORIES(
  ${PYTHON_INCLUDE_DIRS}
  ${SIP_INCLUDE_DIR}
  ${QGIS_INCLUDE_DIR}/qgis
  ${GDAL_INCLUDE_DIR}
  ${Qt5PrintSupport_INCLUDE_DIRS}
)

SET(DEFAULTDOCSTRINGSIGNATURE "%DefaultDocstringSignature \"prepended\"")
SET(DOCSTRINGSTEMPLATE "")
SET(SIP_FINAL "final")
SET(SIP_INCLUDES ${PYQT5_SIP_DIR} ${QGIS_SIP_DIR} ${CMAKE_CURRENT_BINARY_DIR})

if((${PYQT5_VERSION_STR} VERSION_EQUAL 5.15) OR (${PYQT5_VERSION_STR} VERSION_GREATER 5.15))
  set(SIP_DISABLE_FEATURES ${SIP_DISABLE_FEATURES} VECTOR_MAPPED_TYPE)
endif()

# Macro to byte-compile a target's staged Python resource(s)
macro(PY_COMPILE TARGET_NAME RESOURCE_PATHS)
  if(WITH_PY_COMPILE)
    add_custom_command(TARGET ${TARGET_NAME}
      POST_BUILD
      COMMAND ${Python_EXECUTABLE} -m compileall -q ${RESOURCE_PATHS}
      WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
      COMMENT "Byte-compiling staged resource..."
    )
  endif()
endmacro(PY_COMPILE)

separate_arguments(PYQT5_SIP_FLAGS_args UNIX_COMMAND "${PYQT5_SIP_FLAGS}")

# core module
FILE(GLOB_RECURSE sip_files_core kadascore/*.sip kadascore/*.sip.in)
SET(SIP_EXTRA_FILES_DEPEND ${sip_files_core})
SET(SIP_EXTRA_OPTIONS ${PYQT5_SIP_FLAGS_args} -g -o -a ${CMAKE_BINARY_DIR}/python/kadas.kadascore.api -n ${PYQT5_SIP_IMPORT} -y ${KADAS_PYTHON_OUTPUT_DIRECTORY}/_kadascore.pyi)
GENERATE_SIP_PYTHON_MODULE_CODE(kadas._kadascore kadascore/kadascore.sip "${sip_files_core}" cpp_files)
BUILD_SIP_PYTHON_MODULE(kadas._kadascore kadascore/kadascore.sip ${cpp_files} "" kadas_core ${QGIS_CORE_LIBRARY})

# analysis module
FILE(GLOB_RECURSE sip_files_analysis kadasanalysis/*.sip kadasanalysis/*.sip.in)
SET(SIP_EXTRA_FILES_DEPEND ${sip_files_analysis})
SET(SIP_EXTRA_OPTIONS ${PYQT5_SIP_FLAGS_args} -g -o -a ${CMAKE_BINARY_DIR}/python/kadas.kadasanalysis.api -n ${PYQT5_SIP_IMPORT} -y ${KADAS_PYTHON_OUTPUT_DIRECTORY}/_kadasanalysis.pyi)
GENERATE_SIP_PYTHON_MODULE_CODE(kadas._kadasanalysis kadasanalysis/kadasanalysis.sip "${sip_files_analysis}" cpp_files)
BUILD_SIP_PYTHON_MODULE(kadas._kadasanalysis kadasanalysis/kadasanalysis.sip ${cpp_files} "" kadas_core kadas_analysis ${QGIS_CORE_LIBRARY} ${QGIS_ANALYSIS_LIBRARY})

# gui module
FILE(GLOB_RECURSE sip_files_gui kadasgui/*.sip kadasgui/*.sip.in)
SET(SIP_EXTRA_FILES_DEPEND ${sip_files_gui})
SET(SIP_EXTRA_OPTIONS ${PYQT5_SIP_FLAGS_args} -g -o -a ${CMAKE_BINARY_DIR}/python/kadas.kadasgui.api -n ${PYQT5_SIP_IMPORT} -y ${KADAS_PYTHON_OUTPUT_DIRECTORY}/_kadasgui.pyi)
GENERATE_SIP_PYTHON_MODULE_CODE(kadas._kadasgui kadasgui/kadasgui.sip "${sip_files_gui}" cpp_files)
BUILD_SIP_PYTHON_MODULE(kadas._kadasgui kadasgui/kadasgui.sip ${cpp_files} "" kadas_core kadas_gui ${QGIS_CORE_LIBRARY} ${QGIS_GUI_LIBRARY})


SET(PY_MODULES kadascore kadasanalysis kadasgui)
FOREACH(module ${PY_MODULES})
  ADD_CUSTOM_TARGET(py${module} ALL)
  ADD_DEPENDENCIES(py${module} python_module_kadas__${module})

  # concat auto_additions/*.py in _module_/__init__.py
  FILE(GLOB PY_FILES_AUTO_ADDITIONS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${module}/auto_additions/*.py)
  CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/${module}/__init__.py.in ${KADAS_PYTHON_OUTPUT_DIRECTORY}/${module}/__init__.py COPYONLY)
  FOREACH(pyfile ${PY_FILES_AUTO_ADDITIONS})
    file(READ ${pyfile} CONTENTS)
    file(APPEND ${KADAS_PYTHON_OUTPUT_DIRECTORY}/${module}/__init__.py "${CONTENTS}")
  ENDFOREACH(pyfile)
  INSTALL(FILES ${KADAS_PYTHON_OUTPUT_DIRECTORY}/${module}/__init__.py DESTINATION "${KADAS_PYTHON_INSTALL_DIRECTORY}/${module}")

  FILE(GLOB PY_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${module}/*.py)
  INSTALL(FILES ${PY_FILES} DESTINATION "${KADAS_PYTHON_INSTALL_DIRECTORY}/${module}")
  FILE(GLOB PY_FILES_ADDITIONS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${module}/additions/*.py)
  INSTALL(FILES ${PY_FILES_ADDITIONS} DESTINATION "${KADAS_PYTHON_INSTALL_DIRECTORY}/${module}/additions")
  SET(PY_FILES ${PY_FILES} ${PY_FILES_ADDITIONS})
  FOREACH(pyfile ${PY_FILES})
    GET_FILENAME_COMPONENT(subdir ${pyfile} DIRECTORY)
    ADD_CUSTOM_COMMAND(TARGET py${module}
      POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E make_directory "${KADAS_PYTHON_OUTPUT_DIRECTORY}/${subdir}"
      COMMAND ${CMAKE_COMMAND} -E copy ${pyfile} "${KADAS_PYTHON_OUTPUT_DIRECTORY}/${subdir}"
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
      DEPENDS ${PY_FILES}
    )
  ENDFOREACH(pyfile)
  PY_COMPILE(py${module} "${KADAS_PYTHON_OUTPUT_DIRECTORY}/${module}")
ENDFOREACH(module)

