cmake_minimum_required(VERSION 3.10)
project(waxwing)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_COLOR_DIAGNOSTICS ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_EXAMPLES "Build examples targets" ON)
option(ENABLE_TESTING "Build testing target" ON)

add_library(${PROJECT_NAME} STATIC)
target_sources(${PROJECT_NAME} PRIVATE
  src/http.cc
  src/io.cc
  src/request.cc
  src/response.cc
  src/router.cc
  src/server.cc
  src/thread_pool.cc
)

target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

set_target_properties(${PROJECT_NAME} PROPERTIES PUBLIC_HEADER include/server.hh)

find_package(fmt REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC fmt)

find_package(spdlog REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC spdlog::spdlog)

if (ENABLE_TESTING)
  add_subdirectory(tests)
endif(ENABLE_TESTING)

if (BUILD_EXAMPLES)
  add_subdirectory(examples)
endif(BUILD_EXAMPLES)
