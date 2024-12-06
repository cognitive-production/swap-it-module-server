cmake_minimum_required(VERSION 3.15...3.26)
project(swapit_module_server VERSION 0.0.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# swap_server_template
find_package(PkgConfig REQUIRED)
pkg_check_modules(SST REQUIRED swap_server_template)

# open62541
find_package(open62541 1.3.10 REQUIRED)

# nodesets
add_subdirectory(nodesets)

# nanobind
add_subdirectory(deps/nanobind)

add_library(module_server)
set_property(TARGET module_server PROPERTY POSITION_INDEPENDENT_CODE ON)

add_subdirectory(src)

target_link_libraries(module_server
    PRIVATE
        open62541::open62541
        ${SST_LIBRARIES}
        nodesets
)

include(GNUInstallDirs)

target_include_directories(module_server 
    PUBLIC 
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE 
        ${SST_INCLUDE_DIRS}
)

add_subdirectory(python)
