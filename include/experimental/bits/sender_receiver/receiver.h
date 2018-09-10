
#pragma once

#include "concepts_20.h"

#include <type_traits> // std::declval
#include <utility> // std::move

namespace std {
namespace experimental {
inline namespace executors_v1 {
namespace execution {

struct __ignore
{
    template <class... Ts>
    __ignore(Ts&&...) {}
    template <class... Ts>
    void operator()(Ts&&...) const noexcept
    {}
};

template <class F = __ignore>
struct on_done : F
{
    on_done() = default;
    on_done(F f) : F(std::move(f)) {}
};
template <class F>
on_done(F) -> on_done<F>;

template <class F = __ignore>
struct on_error : F
{
    on_error() = default;
    on_error(F f) : F(std::move(f)) {}
};
template <class F>
on_error(F) -> on_error<F>;

template <class F = __ignore>
struct on_value : F
{
    on_value() = default;
    on_value(F f) : F(std::move(f)) {}
};
template <class F>
on_value(F) -> on_value<F>;

template <template <class> class On, class F, class... Rest>
On<F> __select_signal(On<F> on, Rest&&...)
{
    return std::move(on);
}
template <template <class> class On, class F, class... Rest>
On<F> __select_signal(__ignore, On<F> on, Rest&&...)
{
    return std::move(on);
}
template <template <class> class On, class F>
On<F> __select_signal(__ignore, __ignore, On<F> on)
{
    return std::move(on);
}
template <template <class> class On>
On<__ignore> __select_signal(__ignore = {}, __ignore = {}, __ignore = {})
{
    return On<__ignore>{__ignore{}};
}
template <template <class> class On, class... Args>
using __select_signal_t = decltype(__select_signal<On>(declval<Args>()...));

template <class F, template <class> class On>
concept bool _InstanceOf =
    requires (F& f) { {f} -> On<auto>; };

template <class S>
concept bool Signal =
    _InstanceOf<S, on_done> || _InstanceOf<S, on_error> || _InstanceOf<S, on_value>;

template <_InstanceOf<on_done> OnDone,
          _InstanceOf<on_error> OnError,
          _InstanceOf<on_value> OnValue>
  requires Invocable<OnDone&>
struct receiver
{
private:
    [[no_unique_address]] OnDone on_done_{};
    [[no_unique_address]] OnError on_error_{};
    [[no_unique_address]] OnValue on_value_{};
public:
    static constexpr void query(receiver_t)
    {}
    template <class... Ss>
      requires sizeof...(Ss) <= 3 && (Signal<Ss> &&...)
    receiver(Ss... ss)
      : on_done_(__select_signal<on_done>(std::move(ss)...))
      , on_error_(__select_signal<on_error>(std::move(ss)...))
      , on_value_(__select_signal<on_value>(std::move(ss)...))
    {}
    void set_done()
    {
        on_done_();
    }
    template <class E>
      requires Invocable<OnError&, E>
    void set_error(E&& e)
    {
        on_error_((E&&) e);
    }
    template <class... Args>
      requires Invocable<OnValue&, Args...>
    void set_value(Args&&... args)
    {
        on_value_((Args&&) args...);
    }
};
template <class... Ss>
  requires sizeof...(Ss) <= 3 && (Signal<Ss> &&...)
receiver(Ss...) ->
    receiver<
        __select_signal_t<on_done, Ss...>,
        __select_signal_t<on_error, Ss...>,
        __select_signal_t<on_value, Ss...>>;

} // end namespace execution
} // end inline namespace executors_v1
} // end namespace experimental
} // end namespace std