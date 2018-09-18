#ifndef STD_EXPERIMENTAL_BITS_SENDER_RECEIVER_H
#define STD_EXPERIMENTAL_BITS_SENDER_RECEIVER_H

#include <exception>
#include <functional>
#include <type_traits>
#include <memory>
#include <future>
#include <any>


#include <experimental/bits/query.h>

#include "sender_receiver/concepts_20.h"
#include "sender_receiver/property_base.h"
#include "sender_receiver/receiver_concept.h"
#include "sender_receiver/sender_concept.h"
#include "sender_receiver/receiver.h"
#include "sender_receiver/sender.h"
#include "sender_receiver/any_sender.h"
#include "sender_receiver/any_receiver.h"
#include "sender_receiver/transform_sender.h"

namespace std {
namespace experimental {
inline namespace executors_v1 {
namespace execution {


// TODO move this to tests
// namespace __test__
// {
//     struct S
//     {
//         static constexpr void query(receiver_t) noexcept
//         {}
//         void set_value(char const*) {}
//     };
//     void set_done(S) {}
//     void set_error(S, int) {}

//     static_assert((bool) Receiver<S>);
//     static_assert((bool) ReceiverOf<S, int, char const*>);
// } // namespace __test__


template <class To, class From>
_CONCEPT ReceiverFrom = SenderTo<From, To>;

// TODO move this to tests
// namespace __test__
// {
//     struct T
//     {
//         static constexpr sender_desc<int, char const*> query(sender_t) noexcept
//         { return {}; }
//         template <ReceiverOf<int, char const*> To>
//         void submit(To to)
//         {
//             set_value(to, "hello world");
//         }
//     };
//     static_assert((bool) Sender<T>);
//     static_assert((bool) SenderTo<T, S>);
// } // namespace __test__


// template <class Adapt, class From>
// _CONCEPT Adaptor =
//     Sender<From> &&
//     requires (Adapt const &a, From& from)
//     {
//         { a.adapt(from) } -> Sender;
//     };

// template <class Function>
// class basic_adaptor
// {
//     Function f_;
// public:
//     basic_adaptor() = default;
//     constexpr explicit basic_adaptor(Function f)
//       : f_(std::move(f))
//     {}
//     template <Sender From>
//       requires Invocable<Function&, From&> &&
//                Sender<invoke_result_t<Function&, From&>>
//     auto adapt(From& from) const
//     {
//         return f_(from);
//     }
// };
// template <class Function>
// basic_adaptor(Function) -> basic_adaptor<Function>;

template <class Function, class R, class S>
struct __bulk_invoke
{
    Function& fun_;
    size_t m_;
    R* r_;
    S* s_;
    template <class... Args, class RR = R>
      requires Invocable<Function&, size_t, Args..., RR&, S&>
    RR& operator()(Args&&... args)
    {
        (void) std::invoke(fun_, m_, (Args&&) args..., *r_, *s_);
        return *r_;
    }
    template <class... Args>
      requires is_void_v<R> && Invocable<Function&, size_t, Args..., S&>
    void operator()(Args&&... args)
    {
        (void) std::invoke(fun_, m_, (Args&&) args..., *s_);
    }
};
template <class Function, class R, class S>
__bulk_invoke(Function&, size_t, R*, S*) -> __bulk_invoke<Function, R, S>;

template <class Function, class RF, class SF>
  requires Invocable<RF&> && Invocable<SF&>
using __bulk_invoke_t = __bulk_invoke<Function, invoke_result_t<RF&>, invoke_result_t<SF&>>;

inline constexpr auto const __to_any = [](auto&&... a) { return any{(decltype(a)&&) a...}; };

} // namespace execution
} // inline namespace executors_v1
} // namespace experimental
} // namespace std

#endif // STD_EXPERIMENTAL_BITS_SENDER_RECEIVER_H
