cmake_minimum_required(VERSION 3.10)
project(server)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_COLOR_DIAGNOSTICS ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_EXAMPLE "Build example" ON)
option(ENABLE_TESTING "Build testing target" ON)

add_library(${PROJECT_NAME} STATIC)
target_sources(${PROJECT_NAME} PRIVATE
  src/io.cc
  src/request.cc
  src/response.cc
  src/router.cc
  src/server.cc
  src/str_util.cc
)

target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

set_target_properties(${PROJECT_NAME} PROPERTIES PUBLIC_HEADER include/server.hh)

if (ENABLE_TESTING)
  add_subdirectory(tests)
endif(ENABLE_TESTING)

if (BUILD_EXAMPLE)
  add_subdirectory(example)
endif(BUILD_EXAMPLE)
