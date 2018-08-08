#ifndef STD_EXPERIMENTAL_BITS_IS_EXECUTOR_H
#define STD_EXPERIMENTAL_BITS_IS_EXECUTOR_H

#include <type_traits>
#include <utility>

namespace std {
namespace experimental {
inline namespace executors_v1 {
namespace execution {
namespace is_executor_impl {

template <class T, class E>
struct _exemplar_void_continuation {
	void value(T&&) && { }
	void error(E&&) && { }
};
template <class E>
struct _exemplar_void_continuation<void, E> {
	void value() && { }
	void error(E&&) && { }
};

template<class Executor, class T, class E, class = std::void_t<>>
struct eval : std::false_type {};

template<class Executor, class T, class E>
struct eval<Executor, T, E,
  void_t<
    enable_if_t<is_nothrow_move_constructible<Executor>::value>,
    enable_if_t<noexcept(static_cast<bool>(declval<const Executor&>() == declval<const Executor&>()))>,
    enable_if_t<noexcept(static_cast<bool>(declval<const Executor&>() != declval<const Executor&>()))>,
		enable_if_t<is_same_v<void, decltype(
			declval<Executor&&>().execute(declval<_exemplar_void_continuation<T, E>>())
		)>>
	>
> : std::true_type { };

} // namespace is_executor_impl

template<class Executor, class T, class E>
struct is_executor : is_executor_impl::eval<Executor, T, E> { };

} // namespace execution
} // inline namespace executors_v1
} // namespace experimental
} // namespace std

#endif // STD_EXPERIMENTAL_BITS_IS_EXECUTOR_H
