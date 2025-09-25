
set(main_SRC
  main.cpp
)

if(WIN32)
  set(SHOW_CONSOLE
      OFF
      CACHE BOOL "Show console (Windows)"
  )
  if(NOT SHOW_CONSOLE)
    set(WIN32_SHOW_CONSOLE "WIN32")
  endif()
endif(WIN32)

if(WIN32)
  enable_language(RC)
  list(APPEND main_SRC "${CMAKE_SOURCE_DIR}/kadas/resources/kadas-icon.rc")
endif()

add_executable(kadas ${WIN32_SHOW_CONSOLE} ${main_SRC})

target_link_libraries(kadas QGIS::Core QGIS::Gui kadas_app kadas_core)

target_include_directories(kadas PUBLIC ${CMAKE_SOURCE_DIR}/kadas/main)

install(TARGETS kadas DESTINATION bin)
