add_subdirectory(core)
add_subdirectory(analysis)
add_subdirectory(gui)
add_subdirectory(app)
add_subdirectory(main)

if(BUILD_TESTING)
  add_subdirectory(test)
endif()

install(FILES resources/timezone21.bin DESTINATION share/kadas/resources)
