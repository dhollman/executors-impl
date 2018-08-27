#ifndef STD_EXPERIMENTAL_BITS_IS_ONEWAY_EXECUTOR_H
#define STD_EXPERIMENTAL_BITS_IS_ONEWAY_EXECUTOR_H

#include <experimental/bits/is_executor.h>
#include <experimental/bits/require.h>
#include <experimental/bits/sender_receiver.h>

namespace std {
namespace experimental {
inline namespace executors_v1 {
namespace execution {
namespace is_oneway_executor_impl {

struct nullary_function
{
  void operator()() {}
};

template<class T>//, class = std::void_t<>>
struct eval : std::false_type {};

template<class T>
  requires Same<T, invoke_result_t<require_impl::require_fn, T&, oneway_t>>
struct eval<T> : is_executor_impl::eval<T> {};


// template<class T>
// struct eval<T,
//   std::void_t<
//     typename std::enable_if<std::is_same<void, decltype(std::declval<const T&>().execute(std::declval<nullary_function>()))>::value>::type,
//     typename std::enable_if<std::is_same<void, decltype(std::declval<const T&>().execute(std::declval<nullary_function&>()))>::value>::type,
//     typename std::enable_if<std::is_same<void, decltype(std::declval<const T&>().execute(std::declval<const nullary_function&>()))>::value>::type,
//     typename std::enable_if<std::is_same<void, decltype(std::declval<const T&>().execute(std::declval<nullary_function&&>()))>::value>::type
// 	>> : is_executor_impl::eval<T> {};

} // namespace is_oneway_executor_impl

template<class Executor>
struct is_oneway_executor : is_oneway_executor_impl::eval<Executor> {};

} // namespace execution
} // inline namespace executors_v1
} // namespace experimental
} // namespace std

#endif // STD_EXPERIMENTAL_BITS_IS_ONEWAY_EXECUTOR_H
