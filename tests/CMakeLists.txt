find_package(GTest REQUIRED)
include(GoogleTest)
enable_testing()

function(add_test_target NAME SRC)
  add_executable(
    ${NAME}
    ${SRC}
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
endfunction()

add_test_target(test 
  str_utils.cc
)
