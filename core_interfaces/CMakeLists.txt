
add_library(core_interfaces INTERFACE)
target_include_directories(core_interfaces
  INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

