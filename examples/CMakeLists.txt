function(add_example NAME SRC)
  add_executable(${NAME} ${SRC})
  target_include_directories(${NAME} PRIVATE ${CMAKE_SOURCE_DIR}/include)
  target_link_libraries(${NAME} PRIVATE ${PROJECT_NAME})

  target_compile_options(${NAME} PRIVATE -fsanitize=address,undefined)
  target_link_options(${NAME} PRIVATE -fsanitize=address,undefined)
endfunction()

add_example(hello_world hello_world.cc)
add_example(headers headers.cc)
add_example(methods methods.cc)
add_example(path_parameters path_parameters.cc)
