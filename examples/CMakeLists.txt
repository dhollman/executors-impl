
macro(add_example name sources)
  add_executable(${name} ${sources})
  target_link_libraries(${name} std::executors)
endmacro()

add_subdirectory(adapter)
