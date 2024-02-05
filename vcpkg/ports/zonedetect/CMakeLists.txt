cmake_minimum_required(VERSION 3.3.0)

project( ZoneDetect )

set(SRC_FILES library/zonedetect.c)
set(HDR_FILES library/zonedetect.h)

add_library( ${PROJECT_NAME}
    ${HDR_FILES}
    ${SRC_FILES}
)
set_property( TARGET ${PROJECT_NAME}
    PROPERTY PUBLIC_HEADER ${HDR_FILES}
)
target_include_directories( ${PROJECT_NAME}
    PRIVATE ${CMAKE_CURRENT_LIST_DIR}
    INTERFACE $<INSTALL_INTERFACE:include>
)
install( TARGETS ${PROJECT_NAME}
    EXPORT unofficial-zonedetect-config
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    PUBLIC_HEADER DESTINATION include/zonedetect
)
install( EXPORT unofficial-zonedetect-config
    FILE unofficial-zonedetect-config.cmake
    NAMESPACE unofficial::zonedetect::
    DESTINATION share/unofficial-zonedetect
)
