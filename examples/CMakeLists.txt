
macro(add_example name sources)
  add_executable(${name} ${sources})
  target_link_libraries(${name} std::executors)
endmacro()

#add_subdirectory(adapter)
add_subdirectory(then_adapter)
add_subdirectory(openmp_bulk)
