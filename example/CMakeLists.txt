find_package(fmt REQUIRED)

add_executable(main main.cc)
message(${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(
  main PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(main PRIVATE
  fmt::fmt
  ${PROJECT_NAME}
)
