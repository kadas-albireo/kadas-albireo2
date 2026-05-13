# Standalone diagnostic tool: probe a URL with QNetworkAccessManager the way
# Kadas does, to compare against a browser. Not a unit test (interactive /
# network-dependent), so not registered with ctest.
#
# Run: <build>/output/bin/qt_portal_probe <url> [--system-proxy] [--user U
# --pass P] [--verbose]
add_executable(qt_portal_probe main.cpp)
set_target_properties(
  qt_portal_probe PROPERTIES RUNTIME_OUTPUT_DIRECTORY
                             "${CMAKE_BINARY_DIR}/output/bin"
)
target_link_libraries(qt_portal_probe PRIVATE Qt6::Core Qt6::Network)
