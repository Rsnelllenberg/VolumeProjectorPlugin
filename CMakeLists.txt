cmake_minimum_required(VERSION 3.17)

if(NOT DEFINED MV_EXAMPLES_USE_VCPKG)
    set(MV_EXAMPLES_USE_VCPKG OFF)
elseif(MV_EXAMPLES_USE_VCPKG)
    message(STATUS "DVRPlugins: Using vcpkg to install dependencies")
endif()

# -----------------------------------------------------------------------------
# ExamplePlugins
# -----------------------------------------------------------------------------
set(PROJECT "DVRPlugins")
PROJECT(${PROJECT})

add_subdirectory(DVRVolumeLoader)
add_subdirectory(DVRViewPlugin)
add_subdirectory(DVRTransferFunction)


set_target_properties(DVRVolumeLoader DVRViewPlugin DVRTransferFunction
    PROPERTIES
    FOLDER DVRPlugins
)