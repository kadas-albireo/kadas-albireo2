find_package(Python3 3.7 REQUIRED COMPONENTS Interpreter Development)

set(kadas_SRC
    main.cpp
)

set(KADAS_SPLASH
    "splash.png"
    CACHE STRING "Splash image"
)

if(WIN32)
  enable_language(RC)
  list(APPEND kadas_SRC "${CMAKE_SOURCE_DIR}/kadas/resources/kadas-icon.rc")
endif()
if(MINGW)
  set(CMAKE_RC_COMPILER_INIT ${CMAKE_GENERATOR_RC})
  set(CMAKE_RC_COMPILE_OBJECT
      "<CMAKE_RC_COMPILER> -O coff -i <SOURCE> -o <OBJECT>"
  )
  set(kadas_RC_LIBS -mwindows)
endif(MINGW)

if(WIN32)
  find_package(Qt5AxContainer REQUIRED)
  set(AXCONTAINER_LIBRARY Qt5::AxContainer)

  set(SHOW_CONSOLE
      OFF
      CACHE BOOL "Show console (Windows)"
  )
  if(NOT SHOW_CONSOLE)
    set(WIN32_SHOW_CONSOLE "WIN32")
  endif()
endif(WIN32)

add_executable(
  kadas
  ${WIN32_SHOW_CONSOLE}
  ${kadas_SRC}
)

target_link_libraries(
  kadas
  Qt5::Widgets
  Qt5::Network
  Qt5::Xml
  Qt5::Sql
  Qt5::OpenGL
  Qt5::PrintSupport
  QuaZip::QuaZip
  ${AXCONTAINER_LIBRARY}
  QGIS::Core
  QGIS::Gui
  QGIS::3D
  Python3::Python
  ${kadas_RC_LIBS}
  kadas_app
  kadas_core
  kadas_analysis
  kadas_gui
)

target_include_directories(
  kadas
  PUBLIC ${CMAKE_SOURCE_DIR}/kadas/main
)

if(WITH_BINDINGS)
  add_definitions(-DWITH_BINDINGS)
endif()

install(TARGETS kadas DESTINATION bin)

