
#pragma once

#include "concepts_20.h"
#include "property_base.h"

#include <future> // std::promise<T>

namespace std {
namespace experimental {
inline namespace executors_v1 {
namespace execution {

struct receiver_t : __property_base<false, false>
{
    template <class T>
    friend constexpr void query(promise<T>&, receiver_t)
    {}
};

template <class To>
_CONCEPT _Receiver =
    Invocable<query_impl::query_fn const&, To&, receiver_t>;

inline constexpr struct set_done_fn
{
    template <class T>
    constexpr void operator()(std::promise<T>& to) const
        noexcept(noexcept(to.set_exception(make_exception_ptr(logic_error{""}))))
    {
        (void) to.set_exception(make_exception_ptr(
            logic_error{"std::promise doesn't support set_done"}));
    }
    template <_Receiver To>
      requires requires (To&& to) { ((To&&) to).set_done(); }
    constexpr void operator()(To&& to) const noexcept(noexcept(((To&&)to).set_done()))
    {
        (void) ((To&&) to).set_done();
    }
    template <_Receiver To>
      requires requires (To&& to) { set_done((To&&) to); }
    constexpr void operator()(To&& to) const volatile noexcept(noexcept(set_done((To&&) to)))
    {
        (void) set_done((To&&) to);
    }
} const set_done {};

template <class To>
_CONCEPT Receiver =
    _Receiver<To> && Invocable<set_done_fn const&, To>;

inline constexpr struct set_value_fn
{
    // This works for std::promise<T> and std::promise<void> also:
    template <Receiver To, class... Args>
      requires requires (To&& to, Args&&... args) { ((To&&) to).set_value((Args&&) args...); }
    void operator()(To&& to, Args&&... args) const
        noexcept(noexcept(((To&&) to).set_value((Args&&) args...)))
    {
        (void) ((To&&) to).set_value((Args&&) args...);
    }
    template <Receiver To, class... Args>
      requires requires (To&& to, Args&&... args) { set_value((To&&) to, (Args&&) args...); }
    void operator()(To&& to, Args&&... args) const
        noexcept(noexcept(set_value((To&&) to, (Args&&) args...)))
    {
        (void) set_value((To&&)to, (Args&&) args...);
    }
} const set_value {};

inline constexpr struct set_error_fn
{
    template <class T>
    void operator()(std::promise<T>& to, exception_ptr e) const
        noexcept(noexcept(to.set_exception(std::move(e))))
    {
        to.set_exception(std::move(e));
    }
    template <class T, class U>
    void operator()(std::promise<T>& to, U&& u) const
        noexcept(noexcept(to.set_exception(make_exception_ptr((U&&) u))))
    {
        to.set_exception(make_exception_ptr((U&&) u));
    }
    template <Receiver To, class E>
      requires requires (To&& to, E&& e) { ((To&&)to).set_error((E&&) e); }
    void operator()(To&& to, E&& e) const noexcept(noexcept(((To&&)to).set_error((E&&) e)))
    {
        ((To&&)to).set_error((E&&) e);
    }
    template <Receiver To, class E>
      requires requires (To&& to, E&& e) { set_error((To&&) to, (E&&) e); }
    void operator()(To&& to, E&& e) const volatile noexcept(noexcept(set_error((To&&) to, (E&&) e)))
    {
        set_error((To&&) to, (E&&) e);
    }
} const set_error {};

template <class To, class E = exception_ptr, class... Args>
_CONCEPT ReceiverOf =
    Receiver<To> &&
    Invocable<set_error_fn const&, To, E> &&
    Invocable<set_value_fn const&, To, Args...>;

} // end namespace execution
} // end inline namespace executors_v1
} // end namespace experimental
} // end namespace std