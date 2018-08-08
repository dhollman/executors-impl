#ifndef STD_EXPERIMENTAL_BITS_IS_THEN_EXECUTOR_H
#define STD_EXPERIMENTAL_BITS_IS_THEN_EXECUTOR_H

#include <experimental/bits/is_executor.h>

#include <type_traits>
#include <utility>

namespace std {
namespace experimental {
inline namespace executors_v1 {
namespace execution {
namespace is_then_executor_impl {

template <class T, class E>
struct _exemplar_continuation {
  void value(T&&) && { }
  void error(E&&) && { }
};

template <class E>
struct _exemplar_continuation<void, E> {
  void value() && { }
  void error(E&&) && { }
};

template<class Executor, class T, class E, class = std::void_t<>>
struct eval : std::false_type {};

template<class Executor, class T, class E>
struct eval<Executor, T, E,
  void_t<
		enable_if_t<is_void_v<void_t<decltype(
			declval<Executor&&>().then_execute(declval<_exemplar_continuation<T, E>>())
		)>>>
	>
> : is_executor<Executor, T, E> { };

} // namespace is_then_executor_impl

template<class Executor, class T, class E>
struct is_then_executor : is_then_executor_impl::eval<Executor, T, E> { };

} // namespace execution
} // inline namespace executors_v1
} // namespace experimental
} // namespace std

#endif // STD_EXPERIMENTAL_BITS_IS_THEN_EXECUTOR_H
