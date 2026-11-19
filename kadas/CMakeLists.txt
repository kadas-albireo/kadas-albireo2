add_subdirectory(core)
add_subdirectory(analysis)
add_subdirectory(gui)
add_subdirectory(app)

install(FILES resources/timezone21.bin
        DESTINATION ${KADAS_DATA_SUBDIR}/resources
)
