cmake_minimum_required(VERSION 3.15...3.26)
project(${SKBUILD_PROJECT_NAME} LANGUAGES CXX)

find_package(Python 3.8 REQUIRED COMPONENTS Interpreter Development.Module)

nanobind_add_module(_module_server_impl NB_STATIC bindings.cpp)

target_link_libraries(_module_server_impl PRIVATE module_server)

if (SKBUILD)
    install(TARGETS _module_server_impl LIBRARY DESTINATION ${SKBUILD_PROJECT_NAME})
endif()