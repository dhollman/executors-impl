
#pragma once

#include "concepts_20.h"
#include "receiver_concept.h"

#include <exception> // std::exception_ptr
#include <utility> // std::move

namespace std {
namespace experimental {
inline namespace executors_v1 {
namespace execution {

template <class Error = exception_ptr, class... Args>
struct on_submit_fn
{
private:
    template <class OnSubmit>
    struct __fn
    {
        using sender_desc_t = sender_desc<Error, Args...>;
        [[no_unique_address]] OnSubmit on_submit_;
        template <Receiver To>
            requires Invocable<OnSubmit&, To>
        void operator()(To to)
        {
            on_submit_(std::move(to));
        }
    };
public:
    template <class OnSubmit>
    auto operator()(OnSubmit on_submit) const
    {
        return __fn<OnSubmit>{std::move(on_submit)};
    }
};
template <class Error = exception_ptr, class... Args>
inline constexpr on_submit_fn<Error, Args...> const on_submit {};

struct __noop_submit
{
    using sender_desc_t = sender_desc<>;
    template <Receiver To>
      requires Invocable<set_value_fn const&, To&>
    void operator()(To to)
    {
        set_value(to);
    }
};
struct __nope {};
template <class OnSubmit, class OnExecutor = __nope>
//  requires requires { typename OnSubmit::sender_desc_t; }
struct sender
{
private:
    [[no_unique_address]] OnSubmit on_submit_;
    [[no_unique_address]] OnExecutor on_executor_;
public:
    sender() = default;
    sender(OnSubmit on_submit) : on_submit_(std::move(on_submit))
    {}
    sender(OnSubmit on_submit, OnExecutor on_executor)
      : on_submit_(std::move(on_submit)), on_executor_(std::move(on_executor))
    {}
    template <std::Same<OnSubmit> OnSubmitLazy>
    friend constexpr typename OnSubmitLazy::sender_desc_t query(sender<OnSubmitLazy, OnExecutor> const&, sender_t) noexcept
    { return {}; }
    template <Receiver To>
      requires Invocable<OnSubmit&, To>
    void submit(To to)
    {
        on_submit_(std::move(to));
    }
    auto executor() const requires _NonVoidInvocable<OnExecutor const&>
    {
        return on_executor_();
    }
};

sender() -> sender<__noop_submit>;

} // end namespace execution
} // end inline namespace executors_v1
} // end namespace experimental
} // end namespace std