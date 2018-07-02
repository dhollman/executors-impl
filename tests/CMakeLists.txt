
macro(add_test name)
  add_executable(${name} ${name}.cpp)
  target_link_libraries(${name} std::executors)
endmacro()

add_test(executor)
add_test(future)
add_test(static_thread_pool)

