#pragma once

#include "concepts_20.h"
#include "property_base.h"

namespace std {
namespace experimental {
inline namespace executors_v1 {
namespace execution {

template <class Error = exception_ptr, class... Args>
struct sender_desc
{
    using error_type = Error;
    template <template <class...> class List>
    using value_types = List<Args...>;
};

struct sender_t : __property_base<false, false> {};

template <class From>
concept bool _Sender =
    requires (From& from)
    {
        { query_impl::query_fn{}(from, sender_t{}) }; // This specificity seems unnecessary: -> sender_desc<auto, auto...>;
    };

namespace __get_executor
{
template <_Sender From>
auto get_executor(From from)
{
    return std::move(from);
}
struct __fn
{
    template <_Sender From>
      requires requires (From& from) { { std::move(from).executor() } -> auto; }
    auto operator()(From from) const
    {
        return std::move(from).executor();
    }
    template <_Sender From>
      requires requires (From& from) { { get_executor(std::move(from)) } -> auto; }
    auto operator()(From from) const volatile
    {
        return get_executor(std::move(from));
    }
};
}
inline constexpr struct get_executor_fn : __get_executor::__fn {} const get_executor {};

template <class From>
concept bool Sender =
    _Sender<From> && Invocable<get_executor_fn const&, From&>;

template <Sender From>
using sender_desc_t = invoke_result_t<query_impl::query_fn const&, From&, sender_t>;

template <Sender From>
struct sender_traits : sender_desc_t<From>
{};

inline constexpr struct submit_fn
{
    template <_Sender From, Receiver To>
      requires requires (From&& from, To to) { ((From&&) from).submit((To&&) to); }
    void operator()(From&& from, To to) const
        noexcept(noexcept(((From&&) from).submit((To&&) to)))
    {
        (void) ((From&&) from).submit((To&&) to);
    }
    template <_Sender From, Receiver To>
      requires requires (From&& from, To to) { submit((From&&) from, (To&&) to); }
    void operator()(From&& from, To to) const volatile
        noexcept(noexcept(submit((From&&) from, (To&&) to)))
    {
        (void) submit((From&&) from, (To&&) to);
    }
} const submit {};

template <class From, class To>
concept bool SenderTo =
    Sender<From> && Receiver<To> && Invocable<submit_fn const&, From, To>;

} // end namespace execution
} // end inline namespace executors_v1
} // end namespace experimental
} // end namespace std