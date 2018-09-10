
#pragma once

#include "concepts_20.h"
#include "any_helpers.h"
#include "receiver_concept.h"

#include <exception> // std::exception_ptr
#include <memory> // std::make_unique

namespace std {
namespace experimental {
inline namespace executors_v1 {
namespace execution {

template <class E = std::exception_ptr, class... Args>
struct any_receiver
{
private:
    struct interface : __cloneable<interface>
    {
        virtual void set_done() = 0;
        virtual void set_error(E) = 0;
        virtual void set_value(Args...) = 0;
    };
    template <ReceiverOf<E, Args...> To>
    struct model : interface
    {
        To to_;
        model(To to) : to_(std::move(to)) {}
        void set_done() override { execution::set_done(to_); }
        void set_error(E e) override { execution::set_error(to_, (E&&) e); }
        void set_value(Args... args) override { execution::set_value(to_, (Args&&) args...); }
        unique_ptr<interface> clone() const override { return make_unique<model>(to_); }
    };
    __pimpl_ptr<interface> impl_;
    template <_NotSame_<any_receiver> A> using not_self_t = A;
public:
    any_receiver() = default;
    template <class To>
      requires ReceiverOf<not_self_t<To>, E, Args...>
    any_receiver(To to)
      : impl_(make_unique<model<To>>(std::move(to)))
    {}
    static constexpr void query(receiver_t) noexcept {}
    void set_done() { impl_->set_done(); }
    void set_error(E e) { impl_->set_error((E&&) e); }
    void set_value(Args... args) { impl_->set_value((Args&&) args...); }
    operator bool() const noexcept { return !!impl_; }
};


} // namespace execution
} // inline namespace executors_v1
} // namespace experimental
} // namespace std