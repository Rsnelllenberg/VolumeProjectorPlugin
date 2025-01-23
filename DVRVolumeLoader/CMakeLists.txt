cmake_minimum_required(VERSION 3.17)

# -----------------------------------------------------------------------------
# BinLoader Plugin
# -----------------------------------------------------------------------------
set(DVRVOLUMELOADER "DVRVolumeLoader")
PROJECT(${DVRVOLUMELOADER})

# -----------------------------------------------------------------------------
# CMake Options
# -----------------------------------------------------------------------------
set(CMAKE_AUTOMOC ON)

if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W3 /DWIN32 /EHsc /MP /permissive- /Zc:__cplusplus")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /NODEFAULTLIB:LIBCMT")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MDd")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MD")
endif(MSVC)

# -----------------------------------------------------------------------------
# Dependencies
# -----------------------------------------------------------------------------
find_package(Qt6 COMPONENTS Widgets WebEngineWidgets REQUIRED)

find_package(ManiVault COMPONENTS Core PointData VolumeData CONFIG)

# -----------------------------------------------------------------------------
# Source files
# -----------------------------------------------------------------------------
set(SOURCES
    src/DVRVolumeLoader.h
    src/DVRVolumeLoader.cpp
    src/DVRVolumeLoader.json
)

set(PLUGIN_MOC_HEADERS
    src/DVRVolumeLoader.h
)

source_group( Plugin FILES ${SOURCES})

# -----------------------------------------------------------------------------
# CMake Target
# -----------------------------------------------------------------------------
add_library(${DVRVOLUMELOADER} SHARED ${SOURCES})

qt_wrap_cpp(DVRVOLUMELOADER_MOC ${PLUGIN_MOC_HEADERS} TARGET ${DVRVOLUMELOADER})
target_sources(${DVRVOLUMELOADER} PRIVATE ${DVRVOLUMELOADER_MOC})

# -----------------------------------------------------------------------------
# Target include directories
# -----------------------------------------------------------------------------
target_include_directories(${DVRVOLUMELOADER} PRIVATE "${ManiVault_INCLUDE_DIR}")

# -----------------------------------------------------------------------------
# Target properties
# -----------------------------------------------------------------------------
target_compile_features(${DVRVOLUMELOADER} PRIVATE cxx_std_17)

# -----------------------------------------------------------------------------
# Target library linking
# -----------------------------------------------------------------------------
target_link_libraries(${DVRVOLUMELOADER} PRIVATE Qt6::Widgets)
target_link_libraries(${DVRVOLUMELOADER} PRIVATE Qt6::WebEngineWidgets)
target_link_libraries(${DVRVOLUMELOADER} PRIVATE ManiVault::Core)
target_link_libraries(${DVRVOLUMELOADER} PRIVATE ManiVault::PointData)
target_link_libraries(${DVRVOLUMELOADER} PRIVATE ManiVault::VolumeData)


# -----------------------------------------------------------------------------
# Target installation
# -----------------------------------------------------------------------------
install(TARGETS ${DVRVOLUMELOADER}
    RUNTIME DESTINATION Plugins COMPONENT PLUGINS # Windows .dll
    LIBRARY DESTINATION Plugins COMPONENT PLUGINS # Linux/Mac .so
)

add_custom_command(TARGET ${DVRVOLUMELOADER} POST_BUILD
    DEPENDS DVRVolumeLoader
    COMMAND "${CMAKE_COMMAND}"
        --install ${CMAKE_CURRENT_BINARY_DIR}
        --config $<CONFIGURATION>
        --prefix ${ManiVault_INSTALL_DIR}/$<CONFIGURATION>
)

set_target_properties(${DVRVOLUMELOADER}
    PROPERTIES
    FOLDER LoaderPlugins
)


# -----------------------------------------------------------------------------
# Miscellaneous
# -----------------------------------------------------------------------------
# Automatically set the debug environment (command + working directory) for MSVC
if(MSVC)
    set_property(TARGET ${DVRVOLUMELOADER} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY  $<IF:$<CONFIG:DEBUG>,${ManiVault_INSTALL_DIR}/debug,${ManiVault_INSTALL_DIR}/release>)
    set_property(TARGET ${DVRVOLUMELOADER} PROPERTY VS_DEBUGGER_COMMAND $<IF:$<CONFIG:DEBUG>,"${ManiVault_INSTALL_DIR}/debug/ManiVault Studio.exe","${ManiVault_INSTALL_DIR}/release/ManiVault Studio.exe">)
endif()
