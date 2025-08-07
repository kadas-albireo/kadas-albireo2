add_subdirectory(core)
add_subdirectory(analysis)
add_subdirectory(gui)
add_subdirectory(app)

if(ENABLE_TESTS)
  add_subdirectory(test)
endif()

install(FILES resources/timezone21.bin DESTINATION share/kadas/resources)
