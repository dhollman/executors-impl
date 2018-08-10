#ifndef STD_EXPERIMENTAL_BITS_IS_BULK_CONTINUATION_H
#define STD_EXPERIMENTAL_BITS_IS_BULK_CONTINUATION_H

#include <experimental/bits/is_continuation.h>
#include <experimental/type_traits>

#include <type_traits>
#include <utility>

namespace std {
namespace experimental {
inline namespace executors_v1 {
namespace execution {
namespace is_bulk_continuation_impl {

template<
  class C, class T, class E, class R, class S,
  class Idx, class Shape, class ShVal, class ShErr,
  class RVal, class RErr, class = void_t<>
>
struct eval : std::false_type {};

// Needed since something that meets the requirements of Continuation<T, E, R, S>
// also meets the requirements of Continuation<T, E, void, void> even if
// is_convertible_v<R, void> isn't true.
template <typename From, typename To>
struct _convertible_to_or_to_is_void
  : conditional_t<
      is_void_v<To>,
      true_type,
      is_convertible<From, To>
    >
{ };

template <class C>
using value_shared_factory_archetype = decltype(
  declval<C const&>().value_shared_factory()
);

template <class C>
using value_result_factory_archetype = decltype(
  declval<C const&>().value_result_factory()
);

template <class C>
using error_shared_factory_archetype = decltype(
  declval<C const&>().error_shared_factory()
);

template <class C>
using error_result_factory_archetype = decltype(
  declval<C const&>().error_result_factory()
);

template <class C, class T, class Idx, class ShVal, class RVal>
using value_element_archetype = decltype(
  declval<C const&>().value_element(declval<T const&>(), declval<Idx const&>(), declval<ShVal&>(), declval<RVal&>())
);

template <class C, class Idx, class ShVal, class RVal>
using void_value_element_archetype = decltype(
  declval<C const&>().value_element(declval<Idx const&>(), declval<ShVal&>(), declval<RVal&>())
);

template <class C, class T, class Idx, class ShVal>
using value_element_void_archetype = decltype(
  declval<C const&>().value_element(declval<T const&>(), declval<Idx const&>(), declval<ShVal&>())
);

template <class C, class Idx, class ShVal>
using void_value_element_void_archetype = decltype(
  declval<C const&>().value_element(declval<Idx const&>(), declval<ShVal&>())
);

template <class C, class E, class Idx, class ShErr, class RErr>
using error_element_archetype = decltype(
  declval<C const&>().error_element(declval<E const&>(), declval<Idx const&>(), declval<RErr&>(), declval<ShErr&>())
);

template <class C>
using shape_archetype = decltype(
  declval<C const&>().shape()
);

template<
  class C, class T, class E, class R, class S,
    class Idx, class Shape, class ShVal, class ShErr,
      class RVal, class RErr
>
struct eval<C, T, E, R, S, Idx, Shape, ShVal, ShErr, RVal, RErr,
  // value_element method requirements:
  enable_if_t<
    // shape must be present
    is_detected_convertible_v<Shape, shape_archetype, C>
    &&
    // at least one of value_shared_factory or error_shared_factory must be present:
    (
      // if value_shared_factory() is present, value_element() must be present
      (
        // value_shared_factory method requirements:
        is_detected_convertible_v<ShVal, value_shared_factory_archetype, C>
        // value_result_factory method requirements:
        && (is_void_v<R> || is_detected_convertible_v<RVal, value_result_factory_archetype, C>)
        // value_element requirements
        && (
          is_void_v<T> ?
            (is_void_v<R> ?
              is_detected_v<void_value_element_void_archetype, C, Idx, ShVal>
              : is_detected_v<void_value_element_archetype, C, Idx, ShVal, RVal>
            )
            : (is_void_v<R> ?
              is_detected_v<value_element_void_archetype, C, T, Idx, ShVal>
              : is_detected_v<value_element_archetype, C, T, Idx, ShVal, RVal>
            )
        )
      )
      ||
      // if error_shared_factory() is present, error_element() must be present
      (
        // error_shared_factory method requirements:
        is_detected_convertible_v<ShErr, error_shared_factory_archetype, C>
        // error_result_factory method requirements:
        && is_detected_convertible_v<RErr, error_result_factory_archetype, C>
          // error_element requirements
        && is_detected_v<error_element_archetype, C, E, Idx, ShErr, RErr>
      )
    )
  >
> : is_continuation<C, T, E, R, S>
{ };


} // end namespace is_bulk_continuation_impl

template <class C, class T, class E, class R, class S,
  class Idx, class Shape, class ShVal, class ShErr, class RVal, class RErr
>
struct is_bulk_continuation : is_bulk_continuation_impl::eval<C, T, E, R, S, Idx, Shape, ShVal, ShErr, RVal, RErr> { };

} // end namespace execution
} // end namespace executors_v1
} // end namespace experimental
} // end namespace std

#endif //STD_EXPERIMENTAL_BITS_IS_BULK_CONTINUATION_H
