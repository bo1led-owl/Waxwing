cmake_minimum_required(VERSION 3.10)
project(waxwing)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_COLOR_DIAGNOSTICS ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_EXAMPLES "Build examples targets" OFF)
option(BUILD_TESTS "Build testing target" OFF)
option(ENABLE_CCACHE "Use ccache for compilation" OFF)

add_library(${PROJECT_NAME} STATIC)
target_sources(${PROJECT_NAME} PRIVATE
  src/http.cc
  src/io.cc
  src/request.cc
  src/response.cc
  src/router.cc
  src/server.cc
  src/str_util.cc
  src/thread_pool.cc
)

target_include_directories(${PROJECT_NAME}
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

find_package(fmt REQUIRED)
find_package(spdlog REQUIRED)
target_link_libraries(${PROJECT_NAME} 
  PUBLIC
    fmt::fmt
    spdlog::spdlog
  PRIVATE
    uring
)

if (BUILD_TESTS)
  add_subdirectory(tests)
endif(BUILD_TESTS)

if (BUILD_EXAMPLES)
  add_subdirectory(examples)
endif(BUILD_EXAMPLES)

if (ENABLE_CCACHE)
  find_program(CCACHE_FOUND ccache)
  if(CCACHE_FOUND)
    set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_COMPILER_LAUNCHER ccache)
  else()
    message(cchache not found)
  endif()
endif(ENABLE_CCACHE)
