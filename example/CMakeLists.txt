find_package(fmt REQUIRED)

add_executable(main main.cc)
target_include_directories(
  main PRIVATE ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(main PRIVATE
  fmt::fmt
  ${PROJECT_NAME}
)
