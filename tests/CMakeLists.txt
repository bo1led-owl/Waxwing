find_package(GTest REQUIRED)
include(GoogleTest)
enable_testing()

macro(add_test_target NAME)
  add_executable(
    ${NAME}
    ${ARGN}
  )
  target_include_directories(${NAME}
    PRIVATE 
      ${CMAKE_SOURCE_DIR}/include/${PROJECT_NAME}
      ${CMAKE_SOURCE_DIR}/src/
  )
  target_link_libraries(${NAME}
    PRIVATE
      GTest::gtest_main
      ${PROJECT_NAME}
  )
  gtest_discover_tests(${NAME})
endmacro()

add_test_target(test 
  str_utils.cc
  thread_pool.cc
  router.cc
  result.cc
)

target_compile_options(test PUBLIC -fsanitize=address,undefined)
target_link_options(test PUBLIC -fsanitize=address,undefined)
