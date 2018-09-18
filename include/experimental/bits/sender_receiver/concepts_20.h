
#pragma once

#include <type_traits>

#ifdef __clang__
#  define _CONCEPT concept
#else
// GCC still hasn't updated this for some reason:
#  define _CONCEPT concept bool
#endif

namespace std {

// From C++20:
template <class A, class B>
_CONCEPT Same = std::is_same_v<A, B> && std::is_same_v<B, A>;

template <class A, class B>
_CONCEPT _NotSame_ = !Same<A, B>;

template <class A, class B>
_CONCEPT DerivedFrom = std::is_base_of_v<B, A>;

template <class From, class To>
_CONCEPT ConvertibleTo =
  is_convertible_v<From, To> && requires (From(&from)()) { static_cast<To>(from()); };

template<class F, class... Args>
_CONCEPT Invocable =
  std::is_invocable_v<F&&, Args...>;

template<class F, class... Args>
_CONCEPT _VoidInvocable =
  Invocable<F, Args...> &&
  Same<void const volatile, invoke_result_t<F, Args...> const volatile>;

template<class F, class... Args>
_CONCEPT _NonVoidInvocable =
  Invocable<F, Args...> && !_VoidInvocable<F, Args...>;

template <class...>
struct __typelist;

} // end namespace std