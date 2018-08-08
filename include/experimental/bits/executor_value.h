#ifndef STD_EXPERIMENTAL_BITS_EXECUTOR_VALUE_H
#define STD_EXPERIMENTAL_BITS_EXECUTOR_VALUE_H

#include <cstddef>
#include <type_traits>

namespace std {
namespace experimental {
inline namespace executors_v1 {
namespace execution {
namespace executor_value_impl {

template<class Executor, class = std::void_t<>>
struct eval { };

template<class Executor>
struct eval<Executor, std::void_t<typename Executor::value_type>>
{
  using type = typename Executor::value_type;
};

} // namespace executor_value_impl

template <class Executor>
struct executor_value : executor_value_impl::eval<Executor> { };

} // namespace execution
} // inline namespace executors_v1
} // namespace experimental
} // namespace std

#endif //STD_EXPERIMENTAL_BITS_EXECUTOR_VALUE_H
