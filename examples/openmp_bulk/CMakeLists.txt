

find_package(OpenMP)

if(OPENMP_FOUND)
  add_example(bulk_openmp bulk_openmp.cpp)
  target_compile_options(bulk_openmp PUBLIC "${OpenMP_CXX_FLAGS}")
endif()