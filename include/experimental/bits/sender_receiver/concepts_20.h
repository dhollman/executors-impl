
#pragma once

namespace std {

// From C++20:
template <class A, class B>
concept bool Same = __is_same_as(A, B) && __is_same_as(B, A);

template <class A, class B>
concept bool _NotSame_ = !Same<A, B>;

template <class A, class B>
concept bool DerivedFrom = __is_base_of(B, A);

template <class From, class To>
concept bool ConvertibleTo =
  is_convertible_v<From, To> && requires (From(&from)()) { static_cast<To>(from()); };

template<class F, class... Args>
concept bool Invocable =
    requires(F&& f)
    {
        std::invoke((F&&) f, declval<Args>()...);
    };

template<class F, class... Args>
concept bool _VoidInvocable =
  Invocable<F, Args...> &&
  Same<void const volatile, invoke_result_t<F, Args...> const volatile>;

template<class F, class... Args>
concept bool _NonVoidInvocable =
  Invocable<F, Args...> && !_VoidInvocable<F, Args...>;

} // end namespace std