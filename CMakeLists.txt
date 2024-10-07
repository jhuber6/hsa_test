cmake_minimum_required(VERSION 3.17)

project(hsa_testing LANGUAGES C CXX)

find_package(hsa-runtime64 QUIET 1.2.0 HINTS ${CMAKE_INSTALL_PREFIX} PATHS /opt/rocm)

add_executable(load load.cpp)
target_link_libraries(load PRIVATE hsa-runtime64::hsa-runtime64)

add_subdirectory(device)
