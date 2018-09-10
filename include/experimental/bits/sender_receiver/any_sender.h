
#pragma once

#include "concepts_20.h"
#include "any_helpers.h"
#include "any_receiver.h"

#include <exception> // std::exception_ptr
#include <type_traits> // conditional_t
#include <memory> // std::unique_ptr

namespace std {
namespace experimental {
inline namespace executors_v1 {
namespace execution {

// For type-erasing senders that send type-erased senders that send
// type-erased senders that...
struct _self;

template <class Error = exception_ptr, class... Args>
struct any_sender
{
private:
    template <Sender From>
    struct receiver_of_self
    {
        static constexpr void query(receiver_t) {}
        void set_done();
        void set_error(Error);
        template <class T>
        void set_value(T) requires Same<T, From> || ConvertibleTo<T, any_sender>;
    };
    using sender_desc_t =
        conditional_t<
            is_same_v<__typelist<_self>, __typelist<Args...>>,
            sender_desc<Error, any_sender>,
            sender_desc<Error, Args...>>;
    using receiver_t =
        conditional_t<
            is_same_v<__typelist<_self>, __typelist<Args...>>,
            any_receiver<Error, any_sender>,
            any_receiver<Error, Args...>>;
    template <Sender From>
    using receiver_of_self_t =
        conditional_t<
            is_same_v<__typelist<_self>, __typelist<Args...>>,
            receiver_of_self<From>,
            any_receiver<Error, Args...>>;
    struct interface : __cloneable<interface>
    {
        virtual void submit(receiver_t) = 0;
    };
    template <SenderTo<receiver_t> From>
    struct model : interface
    {
        From from_;
        model(From from) : from_(std::move(from)) {}
        void submit(receiver_t to) override { execution::submit(from_, std::move(to)); }
        unique_ptr<interface> clone() const override { return make_unique<model>(from_); }
    };
    __pimpl_ptr<interface> impl_;
    template <_NotSame_<any_sender> A> using not_self_t = A;
public:
    using receiver_type = receiver_t;
    any_sender() = default;
    template <class From>
      requires SenderTo<not_self_t<From>, receiver_of_self_t<From>>
    any_sender(From from)
      : impl_(make_unique<model<From>>(std::move(from)))
    {}
    static constexpr sender_desc_t query(sender_t) noexcept
    { return {}; }
    void submit(receiver_type to) { impl_->submit(std::move(to)); }
};




} // end namespace execution
} // end inline namespace executors_v1
} // end namespace experimental
} // end namespace std