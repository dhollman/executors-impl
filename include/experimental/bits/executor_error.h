#ifndef STD_EXPERIMENTAL_BITS_EXECUTOR_ERROR_H
#define STD_EXPERIMENTAL_BITS_EXECUTOR_ERROR_H

#include <type_traits> // void_t
#include <exception> // exception_ptr

namespace std {
namespace experimental {
inline namespace executors_v1 {
namespace execution {
namespace executor_error_impl {

template<class Executor, class = std::void_t<>>
struct eval
{
  using type = std::exception_ptr;
};

template<class Executor>
struct eval<Executor, std::void_t<typename Executor::error_type>>
{
  using type = typename Executor::error_type;
};

} // namespace executor_error_impl

template <class Executor>
struct executor_error : executor_error_impl::eval<Executor> { };

} // namespace execution
} // inline namespace executors_v1
} // namespace experimental
} // namespace std

#endif //STD_EXPERIMENTAL_BITS_EXECUTOR_ERROR_H
