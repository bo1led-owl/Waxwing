function(add_example NAME SRC)
  add_executable(${NAME} ${SRC})
  target_include_directories(${NAME} PRIVATE ${CMAKE_SOURCE_DIR}/include)
  target_link_libraries(${NAME} PRIVATE ${PROJECT_NAME})
endfunction()

add_example(hello_world hello_world.cc)
