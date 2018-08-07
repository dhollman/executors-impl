#ifndef STD_EXPERIMENTAL_BITS_IS_CONTINUATION_H
#define STD_EXPERIMENTAL_BITS_IS_CONTINUATION_H

#include <type_traits>
#include <utility>

namespace std {
namespace experimental {
inline namespace executors_v1 {
namespace execution {
namespace is_continuation_impl {

template<class C, class T, class E, class R, class S, class VSubEx, class ESubEx, class = std::void_t<>>
struct eval : std::false_type {};

// Needed since something that meets the requirements of Continuation<T, E, R, S, VSubEx, ESubEx>
// also meets the requirements of Continuation<T, E, void, void, VSubEx, ESubEx> even if
// is_convertible_v<R, void> isn't true.
template <typename From, typename To>
struct _convertible_to_or_to_is_void
  : conditional_t<
      is_void_v<To>,
      true_type,
      is_convertible<From, To>
    >
{ };

template<class C, class T, class E, class R, class S, class VSubEx, class ESubEx>
struct eval<C, T, E, R, S, VSubEx, ESubEx,
  void_t<
    // value method requirements:
    enable_if_t<_convertible_to_or_to_is_void<decltype(
      declval<C&&>().value(declval<T&&>(), declval<VSubEx&&>())
    ), R>::value>,
    // error method requirements:
    enable_if_t<_convertible_to_or_to_is_void<decltype(
      declval<C&&>().error(declval<E&&>(), declval<ESubEx&&>())
    ), R>::value>,
    // also requires move_constructible:
    enable_if_t<is_nothrow_move_constructible<T>::value>
  >
>
{ };

} // end namespace is_continuation_impl

template <class C, class T, class E, class R, class S, class VSubEx, class ESubEx>
struct is_continuation : is_continuation_impl::eval<C, T, E, R, S, VSubEx, ESubEx> { };

} // end namespace execution
} // end namespace executors_v1
} // end namespace experimental
} // end namespace std

#endif //STD_EXPERIMENTAL_BITS_IS_CONTINUATION_H
