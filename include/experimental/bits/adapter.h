#ifndef STD_EXPERIMENTAL_BITS_ADAPTER_H
#define STD_EXPERIMENTAL_BITS_ADAPTER_H

#include <experimental/bits/query_member_traits.h>
#include <experimental/bits/query_static_member_traits.h>
#include <experimental/bits/require_member_traits.h>

#include <experimental/execution>

#include <utility>

namespace std {
namespace experimental {
inline namespace executors_v1 {
namespace execution {
namespace impl {

template <typename Executor, typename Base>
class trivial_adapter_mixin : public Base
{
  private:
    using base_t = Base;
    template <class T> static auto inner_declval() -> decltype(std::declval<Executor>());

  public:

    using base_t::base_t;

    template<class Continuation>
    auto execute(Continuation&& c) const &
      -> decltype(inner_declval<Continuation const&>().execute(std::forward<Continuation&&>(c)))
    {
      return this->executor_.execute(c);
    }

    template<class Continuation>
    auto execute(Continuation&& c) &&
    -> decltype(inner_declval<Continuation>().execute(std::forward<Continuation&&>(c)))
    {
      return std::move(this->executor_).execute(std::forward<Continuation>(c));
    }

    template<class Continuation>
    auto then_execute(Continuation&& c) const &
    -> decltype(inner_declval<Continuation const&>().then_execute(std::forward<Continuation>(c)))
    {
      return this->executor_.then_execute(std::forward<Continuation>(c));
    }

    template<class Continuation>
    auto then_execute(Continuation&& c) &&
    -> decltype(inner_declval<Continuation>().then_execute(std::forward<Continuation>(c)))
    {
      return std::move(this->executor_).then_execute(std::forward<Continuation>(c));
    }

};

template<template<class> class Derived, class Executor>
class adapter
{
public:

  using value_type = executor_value_t<Executor>;
  using error_type = executor_error_t<Executor>;

  adapter(adapter&&) noexcept = default;
  adapter(Executor ex) noexcept
    : executor_(std::move(ex))
  { }

  template<class Property>
  constexpr auto require(const Property& p) const &
    noexcept(require_member_traits<Executor, Property>::is_noexcept)
    -> Derived<typename require_member_traits<Executor, Property>::result_type>
  {
    return Derived<decltype(executor_.require(p))>(executor_.require(p), *this);
  }

  template<class Property>
  constexpr auto require(const Property& p) &&
    noexcept(require_member_traits<Executor, Property>::is_noexcept)
    -> Derived<typename require_member_traits<Executor, Property>::result_type>
  {
    return Derived<decltype(executor_.require(p))>(executor_.require(p),
      static_cast<Derived<Executor>&&>(std::move(*this)));
  }

  template<class Property>
  static constexpr auto query(const Property& p)
    noexcept(query_static_member_traits<Executor, Property>::is_noexcept)
    -> typename query_static_member_traits<Executor, Property>::result_type
  {
    return Executor::query(p);
  }

  template<class Property>
  constexpr auto query(const Property& p) const
    noexcept(query_member_traits<Executor, Property>::is_noexcept)
    -> typename enable_if<
      !query_static_member_traits<Executor, Property>::is_valid,
      typename query_member_traits<Executor, Property>::result_type
    >::type
  {
    return executor_.query(p);
  }

  friend constexpr bool operator==(const Derived<Executor>& a,
      const Derived<Executor>& b) noexcept
  {
    return a.executor_ == b.executor_;
  }

  friend constexpr bool operator!=(const Derived<Executor>& a,
      const Derived<Executor>& b) noexcept
  {
    return a.executor_ != b.executor_;
  }

protected:
  Executor executor_;
};

} // namespace impl
} // namespace execution
} // inline namespace executors_v1
} // namespace experimental
} // namespace std

#endif // STD_EXPERIMENTAL_BITS_ADAPTER_H
