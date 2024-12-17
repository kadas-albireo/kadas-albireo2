find_package(SIP REQUIRED)
find_package(PyQt5 REQUIRED)
include(PythonMacros)
include(SIPMacros)

set(KADAS_PYTHON_OUTPUT_DIRECTORY ${KADAS_OUTPUT_DIRECTORY}/python/kadas)
set(KADAS_PYTHON_INSTALL_DIRECTORY ${SITEARCH_INSTALL_DIR}/kadas)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${KADAS_PYTHON_OUTPUT_DIRECTORY})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${KADAS_PYTHON_OUTPUT_DIRECTORY})

set(DEFAULTDOCSTRINGSIGNATURE "%DefaultDocstringSignature \"prepended\"")
set(DOCSTRINGSTEMPLATE "")
set(SIP_FINAL "final")

if((${PYQT5_VERSION_STR} VERSION_EQUAL 5.15) OR (${PYQT5_VERSION_STR}
                                                 VERSION_GREATER 5.15)
)
  set(SIP_DISABLE_FEATURES ${SIP_DISABLE_FEATURES} VECTOR_MAPPED_TYPE)
endif()
if(NOT ANDROID)
  set(SIP_DISABLE_FEATURES ${SIP_DISABLE_FEATURES} ANDROID)
endif()

# Macro to byte-compile a target's staged Python resource(s)
macro(PY_COMPILE TARGET_NAME RESOURCE_PATHS)
  if(WITH_PY_COMPILE)
    add_custom_command(
      TARGET ${TARGET_NAME}
      POST_BUILD
      COMMAND ${Python_EXECUTABLE} -m compileall -q ${RESOURCE_PATHS}
      WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
      COMMENT "Byte-compiling staged resource..."
    )
  endif()
endmacro(PY_COMPILE)

separate_arguments(PYQT5_SIP_FLAGS_args UNIX_COMMAND "${PYQT5_SIP_FLAGS}")

# core module
file(GLOB_RECURSE sip_files_core kadascore/*.sip kadascore/*.sip.in)
set(SIP_EXTRA_FILES_DEPEND ${sip_files_core})
set(SIP_EXTRA_OPTIONS
    ${PYQT5_SIP_FLAGS_args}
    -g
    -o
    -a
    ${CMAKE_BINARY_DIR}/python/kadas.kadascore.api
    -n
    ${PYQT5_SIP_IMPORT}
    -y
    ${KADAS_PYTHON_OUTPUT_DIRECTORY}/_kadascore.pyi
)
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/kadascore/project.py.in
  ${CMAKE_CURRENT_BINARY_DIR}/kadascore/project.py @ONLY
)
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/kadascore/pyproject.toml.in
  ${CMAKE_CURRENT_BINARY_DIR}/kadascore/pyproject.toml @ONLY
)
generate_sip_python_module_code(
  kadas._kadascore kadascore/kadascore.sip "${sip_files_core}" cpp_files
)
build_sip_python_module(
  kadas._kadascore kadascore/kadascore.sip ${cpp_files} "" kadas_core
  QGIS::Core
)

# analysis module
file(GLOB_RECURSE sip_files_analysis kadasanalysis/*.sip kadasanalysis/*.sip.in)
set(SIP_EXTRA_FILES_DEPEND ${sip_files_analysis})
set(SIP_EXTRA_OPTIONS
    ${PYQT5_SIP_FLAGS_args}
    -g
    -o
    -a
    ${CMAKE_BINARY_DIR}/python/kadas.kadasanalysis.api
    -n
    ${PYQT5_SIP_IMPORT}
    -y
    ${KADAS_PYTHON_OUTPUT_DIRECTORY}/_kadasanalysis.pyi
)
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/kadasanalysis/project.py.in
  ${CMAKE_CURRENT_BINARY_DIR}/kadasanalysis/project.py @ONLY
)
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/kadasanalysis/pyproject.toml.in
  ${CMAKE_CURRENT_BINARY_DIR}/kadasanalysis/pyproject.toml @ONLY
)
generate_sip_python_module_code(
  kadas._kadasanalysis kadasanalysis/kadasanalysis.sip "${sip_files_analysis}"
  cpp_files
)
build_sip_python_module(
  kadas._kadasanalysis
  kadasanalysis/kadasanalysis.sip
  ${cpp_files}
  ""
  kadas_core
  kadas_analysis
  QGIS::Analysis
)

# gui module
file(GLOB_RECURSE sip_files_gui kadasgui/*.sip kadasgui/*.sip.in)
set(SIP_EXTRA_FILES_DEPEND ${sip_files_gui})
set(SIP_EXTRA_OPTIONS
    ${PYQT5_SIP_FLAGS_args}
    -g
    -o
    -a
    ${CMAKE_BINARY_DIR}/python/kadas.kadasgui.api
    -n
    ${PYQT5_SIP_IMPORT}
    -y
    ${KADAS_PYTHON_OUTPUT_DIRECTORY}/_kadasgui.pyi
)
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/kadasgui/project.py.in
  ${CMAKE_CURRENT_BINARY_DIR}/kadasgui/project.py @ONLY
)
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/kadasgui/pyproject.toml.in
  ${CMAKE_CURRENT_BINARY_DIR}/kadasgui/pyproject.toml @ONLY
)
generate_sip_python_module_code(
  kadas._kadasgui kadasgui/kadasgui.sip "${sip_files_gui}" cpp_files
)
build_sip_python_module(
  kadas._kadasgui
  kadasgui/kadasgui.sip
  ${cpp_files}
  ""
  kadas_core
  kadas_gui
  QGIS::Gui
)

set(PY_MODULES kadascore kadasanalysis kadasgui)
foreach(module ${PY_MODULES})
  add_custom_target(py${module} ALL)
  add_dependencies(py${module} python_module_kadas__${module})

  # concat auto_additions/*.py in _module_/__init__.py
  file(
    GLOB PY_FILES_AUTO_ADDITIONS
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    ${module}/auto_additions/*.py
  )
  configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/${module}/__init__.py.in
    ${KADAS_PYTHON_OUTPUT_DIRECTORY}/${module}/__init__.py COPYONLY
  )
  foreach(pyfile ${PY_FILES_AUTO_ADDITIONS})
    file(READ ${pyfile} CONTENTS)
    file(APPEND ${KADAS_PYTHON_OUTPUT_DIRECTORY}/${module}/__init__.py
         "${CONTENTS}"
    )
  endforeach(pyfile)
  install(FILES ${KADAS_PYTHON_OUTPUT_DIRECTORY}/${module}/__init__.py
          DESTINATION "${KADAS_PYTHON_INSTALL_DIRECTORY}/${module}"
  )

  file(
    GLOB PY_FILES
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    ${module}/*.py
  )
  install(FILES ${PY_FILES}
          DESTINATION "${KADAS_PYTHON_INSTALL_DIRECTORY}/${module}"
  )
  file(
    GLOB PY_FILES_ADDITIONS
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    ${module}/additions/*.py
  )
  install(FILES ${PY_FILES_ADDITIONS}
          DESTINATION "${KADAS_PYTHON_INSTALL_DIRECTORY}/${module}/additions"
  )
  set(PY_FILES ${PY_FILES} ${PY_FILES_ADDITIONS})
  foreach(pyfile ${PY_FILES})
    get_filename_component(subdir ${pyfile} DIRECTORY)
    add_custom_command(
      TARGET py${module}
      POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E make_directory
              "${KADAS_PYTHON_OUTPUT_DIRECTORY}/${subdir}"
      COMMAND ${CMAKE_COMMAND} -E copy ${pyfile}
              "${KADAS_PYTHON_OUTPUT_DIRECTORY}/${subdir}"
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
      DEPENDS ${PY_FILES}
    )
  endforeach(pyfile)
  py_compile(py${module} "${KADAS_PYTHON_OUTPUT_DIRECTORY}/${module}")
endforeach(module)
