#ifndef STD_EXPERIMENTAL_BITS_SENDER_RECEIVER_H
#define STD_EXPERIMENTAL_BITS_SENDER_RECEIVER_H

#include <exception>
#include <functional>
#include <type_traits>

#include <experimental/bits/query.h>

namespace std {
// From C++20:
template <class A, class B>
concept bool Same = __is_same_as(A, B) && __is_same_as(B, A);

template <class A, class B>
concept bool _NotSame_ = !Same<A, B>;

template <class A, class B>
concept bool DerivedFrom = __is_base_of(B, A);

template<class F, class... Args>
concept bool Invocable =
    requires(F&& f)
    {
        std::invoke((F&&) f, declval<Args>()...);
    };

template<class F, class... Args>
concept bool _NonVoidInvocable =
  Invocable<F, Args...> &&
  !Same<void const volatile, invoke_result_t<F, Args...> const volatile>;

namespace experimental {
inline namespace executors_v1 {
namespace execution {
template <bool Requirable, bool Preferable>
struct __property_base
{
  static inline constexpr bool const is_requirable = Requirable;
  static inline constexpr bool const is_preferable = Preferable;
};
template <bool Requirable, bool Preferable>
constexpr bool const __property_base<Requirable, Preferable>::is_requirable;
template <bool Requirable, bool Preferable>
constexpr bool const __property_base<Requirable, Preferable>::is_preferable;

inline constexpr struct receiver_t : __property_base<false, false>
{
    using polymorphic_query_result_type = size_t;
    static inline constexpr struct none_t : __property_base<false, false> {
        static inline constexpr size_t const cardinality = 0;
        constexpr operator size_t () const noexcept { return cardinality; }
    } const none{};
    static inline constexpr struct single_t : none_t {
        static inline constexpr size_t const cardinality = 1;
        constexpr operator size_t () const noexcept { return cardinality; }
    } const single{};
} const receiver {};

constexpr receiver_t::none_t const receiver_t::none;
constexpr receiver_t::single_t const receiver_t::single;
constexpr size_t const receiver_t::none_t::cardinality;
constexpr size_t const receiver_t::single_t::cardinality;

template <class To, class Tag = receiver_t::none_t>
concept bool _Receiver =
    DerivedFrom<Tag, receiver_t::none_t> &&
    requires (To& to, Tag t)
    {
        { Tag::cardinality } -> Same<size_t const>&;
        { query_impl::customization_point<>(to, receiver) } -> DerivedFrom<Tag>;
    } &&
    Tag::cardinality <= receiver.single.cardinality;

inline constexpr struct __set_done
{
    template <_Receiver To>
      requires requires (To& to) { to.set_done(); }
    void operator()(To& to) const noexcept(noexcept(to.set_done()))
    {
        to.set_done();
    }
    template <_Receiver To>
      requires requires (To& to) { set_done(to); }
    void operator()(To& to) const volatile noexcept(noexcept(set_done(to)))
    {
        set_done(to);
    }
} const set_done {};

template <class To, class Tag = receiver_t::none_t>
concept bool Receiver =
    _Receiver<To, Tag> &&
    requires (To& to)
    {
        set_done(to);
    };

template <Receiver To>
using receiver_category_t =
    decay_t<decltype(query_impl::customization_point<>(declval<To&>(), receiver))>;

namespace __set_value
{
template <Receiver To>
  requires !Receiver<To, receiver_t::single_t>
void set_value(To& to) noexcept(noexcept(set_done(to)))
{
    set_done(to);
}
struct __fn
{
    template <Receiver To>
      requires requires (To& to) { to.set_value(); } &&
        !Receiver<To, receiver_t::single_t>
    void operator()(To& to) const noexcept(noexcept(to.set_value()))
    {
        to.set_value();
    }
    template <Receiver To>
      requires requires (To& to) { set_value(to); } &&
        !Receiver<To, receiver_t::single_t>
    void operator()(To& to) const volatile noexcept(noexcept(set_value(to)))
    {
        set_value(to);
    }
    template <Receiver<receiver_t::single_t> To, class V>
      requires requires (To& to, V&& v) { to.set_value((V&&) v); }
    void operator()(To& to, V&& v) const noexcept(noexcept(to.set_value((V&&) v)))
    {
        to.set_value((V&&) v);
    }
    template <Receiver<receiver_t::single_t> To, class V>
      requires requires (To& to, V&& v) { set_value(to, (V&&) v); }
    void operator()(To& to, V&& v) const volatile noexcept(noexcept(set_value(to, (V&&) v)))
    {
        set_value(to, (V&&) v);
    }
};
}
inline constexpr __set_value::__fn const set_value {};

inline constexpr struct __set_error
{
    template <Receiver To, class E>
      requires requires (To& to, E&& e) { to.set_error((E&&) e); }
    void operator()(To& to, E&& e) const noexcept(noexcept(to.set_error((E&&) e)))
    {
        to.set_error((E&&) e);
    }
    template <Receiver To, class E>
      requires requires (To& to, E&& e) { set_error(to, (E&&) e); }
    void operator()(To& to, E&& e) const volatile noexcept(noexcept(set_error(to, (E&&) e)))
    {
        set_error(to, (E&&) e);
    }
} const set_error {};

template <class To, class E = exception_ptr>
concept bool _NoneReceiver =
    Receiver<To> &&
    requires (To& to, E&& e)
    {
        set_error(to, static_cast<E&&>(e));
    };

template <class To, class E = exception_ptr>
concept bool NoneReceiver =
    _NoneReceiver<To, E> &&
    requires (To& to)
    {
        set_value(to);
    };

template <class To, class V, class E = exception_ptr>
concept bool SingleReceiver =
    Receiver<To, receiver_t::single_t> &&
    _NoneReceiver<To, E> &&
    requires (To& to, V&& v)
    {
        set_value(to, static_cast<V&&>(v));
    };

namespace __test__
{
    struct S
    {
        static constexpr auto query(receiver_t) noexcept
        {
            return receiver.single;
        }
        void set_value(char const*) {}
    };
    void set_done(S) {}
    void set_error(S, int) {}

    static_assert((bool) Receiver<S>);
    static_assert(!(bool) NoneReceiver<S, int>);
    static_assert((bool) SingleReceiver<S, char const*, int>);
} // namespace __test__

inline constexpr struct sender_t : __property_base<false, false>
{
    using polymorphic_query_result_type = size_t;
    static inline constexpr struct none_t : __property_base<false, false> {
        static inline constexpr size_t const cardinality = 0;
        constexpr operator size_t () const noexcept { return cardinality; }
    } const none{};
    static inline constexpr struct single_t : none_t {
        static inline constexpr size_t const cardinality = 1;
        constexpr operator size_t () const noexcept { return cardinality; }
    } const single{};
} const sender {};

constexpr sender_t::none_t const sender_t::none;
constexpr sender_t::single_t const sender_t::single;
constexpr size_t const sender_t::none_t::cardinality;
constexpr size_t const sender_t::single_t::cardinality;

template <class From, class Tag = sender_t::none_t>
concept bool _Sender =
    DerivedFrom<Tag, sender_t::none_t> &&
    requires (From& from, Tag t)
    {
        { Tag::cardinality } -> Same<size_t const>&;
        { query_impl::customization_point<>(from, sender) } -> DerivedFrom<Tag>;
    } &&
    Tag::cardinality <= sender.single.cardinality;

namespace __get_executor__
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
inline constexpr __get_executor__::__fn const get_executor {};

template <class From, class Tag = sender_t::none_t>
concept bool Sender =
    _Sender<From, Tag> &&
    requires (From& from)
    {
        { get_executor(from) } -> _Sender<Tag>;
    };

template <Sender From>
using sender_category_t =
    decay_t<decltype(query_impl::customization_point<>(declval<From&>(), sender))>;

inline constexpr struct __submit
{
    template <_Sender From, Receiver To>
      requires requires (From& from, To to) { from.submit((To&&) to); } &&
        sender_category_t<From>::cardinality <= receiver_category_t<To>::cardinality
    void operator()(From& from, To to) const noexcept(noexcept(from.submit((To&&) to)))
    {
        from.submit((To&&) to);
    }
    template <_Sender From, Receiver To>
      requires requires (From& from, To to) { submit(from, (To&&) to); } &&
        sender_category_t<From>::cardinality <= receiver_category_t<To>::cardinality
    void operator()(From& from, To to) const volatile noexcept(noexcept(submit(from, (To&&) to)))
    {
        submit(from, (To&&) to);
    }
} const submit {};

template <class From, class To>
concept bool SenderTo =
    Sender<From> && Receiver<To> &&
    requires (From& from, To&& to)
    {
        submit(from, (To&&) to);
    };

template <class To, class From>
concept bool ReceiverFrom = SenderTo<From, To>;

namespace __test__
{
    struct T
    {
        static constexpr auto query(sender_t) noexcept
        {
            return sender.single;
        }
        template <SingleReceiver<char const*, int> To>
        void submit(To to)
        {
            set_value(to, "hello world");
        }
    };
    static_assert(Sender<T>);
    static_assert(SenderTo<T, S>);
} // namespace __test__

struct __nope
{};
inline constexpr struct __ignore_done
{
    void operator()() const noexcept
    {}
} const ignore_done{};
inline constexpr struct __ignore_error
{
    template <class E>
    void operator()(E&&) const noexcept
    {}
} const ignore_error{};
inline constexpr struct __ignore_value
{
    void operator()() const noexcept
    {}
    template <class V>
    void operator()(V&&) const noexcept
    {}
} const ignore_value{};

template <class Tag, class OnDone, class OnError, class OnValue>
  requires Invocable<OnDone&> &&
    (!Same<Tag, sender_t::none_t> || Invocable<OnValue&>)
struct __basic_receiver
{
private:
    [[no_unique_address]] OnDone on_done_{};
    [[no_unique_address]] OnError on_error_{};
    [[no_unique_address]] OnValue on_value_{};
public:
    static constexpr auto query(receiver_t)
    {
        return Tag{};
    }
    __basic_receiver() = default;
    __basic_receiver(OnValue on_value) requires Same<Tag, receiver_t::single_t>
      : on_value_(std::move(on_value))
    {}
    __basic_receiver(OnValue on_value, OnError on_error) requires Same<Tag, receiver_t::single_t>
      : on_error_(std::move(on_error)), on_value_(std::move(on_value))
    {}
    __basic_receiver(OnValue on_value, OnError on_error, OnDone on_done)
        requires Same<Tag, receiver_t::single_t>
      : on_done_(std::move(on_done)), on_error_(std::move(on_error)),
        on_value_(std::move(on_value))
    {}
    __basic_receiver(OnError on_error) requires Same<Tag, receiver_t::none_t>
      : on_error_(std::move(on_error))
    {}
    __basic_receiver(OnError on_error, OnDone on_done)
        requires Same<Tag, receiver_t::none_t>
      : on_done_(std::move(on_done)), on_error_(std::move(on_error))
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
    void set_value() requires Same<Tag, receiver_t::none_t>
    {
        on_done_(); // TODO
    }
    template <class V>
      requires Invocable<OnValue&, V>
    void set_value(V&& v) requires Same<Tag, receiver_t::single_t>
    {
        on_value_((V&&) v);
    }
};

template <class OnError = __ignore_error, class OnDone = __ignore_done>
struct none_receiver : __basic_receiver<receiver_t::none_t, OnDone, OnError, __ignore_value>
{
    none_receiver() = default;
    using none_receiver::__basic_receiver::__basic_receiver;
};
none_receiver() -> none_receiver<>;
template<class OnError>
none_receiver(OnError) -> none_receiver<OnError>;
template<class OnError, class OnDone>
  requires Invocable<OnDone&>
none_receiver(OnError, OnDone) -> none_receiver<OnError, OnDone>;

template <class OnValue = __ignore_value, class OnError = __ignore_error, class OnDone = __ignore_done>
struct single_receiver : __basic_receiver<receiver_t::single_t, OnDone, OnError, OnValue>
{
    single_receiver() = default;
    using single_receiver::__basic_receiver::__basic_receiver;
};
single_receiver() -> single_receiver<>;
template<class OnValue>
single_receiver(OnValue) -> single_receiver<OnValue>;
template<class OnValue, class OnError>
single_receiver(OnValue, OnError) -> single_receiver<OnValue, OnError>;
template<class OnValue, class OnError, class OnDone>
single_receiver(OnValue, OnError, OnDone) -> single_receiver<OnValue, OnError, OnDone>;

template <class Tag, class OnSubmit, class OnExecutor = __nope>
struct __basic_sender
{
private:
    [[no_unique_address]] OnSubmit on_submit_{};
    [[no_unique_address]] OnExecutor on_executor_{};

public:
    __basic_sender() = default;
    __basic_sender(OnSubmit on_submit) : on_submit_(std::move(on_submit))
    {}
    __basic_sender(OnSubmit on_submit, OnExecutor on_executor)
      : on_submit_(std::move(on_submit)), on_executor_(std::move(on_executor))
    {}
    static constexpr auto query(sender_t) noexcept
    {
        return Tag{};
    }
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

struct __noop_submit
{
    template <NoneReceiver To>
    void operator()(To to)
    {
        set_value(to);
    }
};
template <class OnSubmit = __noop_submit, class OnExecutor = __nope>
struct none_sender : __basic_sender<sender_t::none_t, OnSubmit, OnExecutor>
{
    none_sender() = default;
    using none_sender::__basic_sender::__basic_sender;
};
none_sender() -> none_sender<>;
template <class OnSubmit>
none_sender(OnSubmit) -> none_sender<OnSubmit>;
template <class OnSubmit, class OnExecutor>
none_sender(OnSubmit, OnExecutor) -> none_sender<OnSubmit, OnExecutor>;

template <class OnSubmit, class OnExecutor = __nope>
struct single_sender : __basic_sender<sender_t::single_t, OnSubmit, OnExecutor>
{
    single_sender() = default;
    using single_sender::__basic_sender::__basic_sender;
};

template <class OnSubmit>
single_sender(OnSubmit) -> single_sender<OnSubmit>;
template <class OnSubmit, class OnExecutor>
single_sender(OnSubmit, OnExecutor) -> single_sender<OnSubmit, OnExecutor>;

template <class Derived>
struct __cloneable
{
    virtual ~__cloneable() {}
    virtual unique_ptr<Derived> clone() const = 0;
};

template <class Interface>
  requires is_abstract_v<Interface> && requires (Interface const* p) {
      { p->clone() } -> unique_ptr<Interface>;
  }
class __pimpl_ptr
{
    unique_ptr<Interface> ptr_;
public:
    __pimpl_ptr() = default;
    __pimpl_ptr(unique_ptr<Interface> ptr) : ptr_(std::move(ptr)) {}
    __pimpl_ptr(__pimpl_ptr&&) = default;
    __pimpl_ptr(__pimpl_ptr const& that)
      : ptr_(that.ptr_ ? that.ptr_->clone() : unique_ptr<Interface>{}) {}
    __pimpl_ptr& operator=(__pimpl_ptr&&) = default;
    __pimpl_ptr& operator=(__pimpl_ptr const& that)
    {
        if (&that != this)
            ptr_ = (that.ptr_ ? that.ptr_->clone() : unique_ptr<Interface>{});
        return *this;
    }
    Interface* operator->() noexcept { return ptr_.get(); }
    Interface const* operator->() const noexcept { return ptr_.get(); }
    Interface& operator*() noexcept { return *ptr_; }
    Interface const& operator*() const noexcept { return *ptr_; }
};

template <class E = std::exception_ptr>
struct any_none_receiver
{
private:
    struct interface : __cloneable<interface>
    {
        virtual void set_done() = 0;
        virtual void set_error(E) = 0;
    };
    template <NoneReceiver<E> To>
    struct model : interface
    {
        To to_;
        model(To to) : to_(std::move(to)) {}
        void set_done() override { execution::set_done(to_); }
        void set_error(E e) override { execution::set_error(to_, std::move(e)); }
        unique_ptr<interface> clone() const override { return make_unique<model>(to_); }
    };
    __pimpl_ptr<interface> impl_;
    template <_NotSame_<any_none_receiver> A> using not_self_t = A;
public:
    any_none_receiver() = default;
    template <class To>
      requires NoneReceiver<not_self_t<To>, E>
    any_none_receiver(To to)
      : impl_(make_unique<model<To>>(std::move(to)))
    {}
    static constexpr auto query(receiver_t) noexcept { return receiver.none; }
    void set_done() { impl_->set_done(); }
    void set_error(E e) { impl_->set_error(std::move(e)); }
};

template <class E = std::exception_ptr>
struct any_none_sender
{
private:
    struct interface : __cloneable<interface>
    {
        virtual void submit(any_none_receiver<E> to) = 0;
    };
    template <SenderTo<any_none_receiver<E>> From>
    struct model : interface
    {
        From from_;
        model(From from) : from_(std::move(from)) {}
        void submit(any_none_receiver<E> to) override { execution::submit(from_, std::move(to)); }
        unique_ptr<interface> clone() const override { return make_unique<model>(from_); }
    };
    __pimpl_ptr<interface> impl_;
    template <_NotSame_<any_none_sender> A> using not_self_t = A;
public:
    any_none_sender() = default;
    template <class From>
      requires SenderTo<not_self_t<From>, any_none_receiver<E>>
    any_none_sender(From from)
      : impl_(make_unique<model<From>>(std::move(from)))
    {}
    static constexpr auto query(sender_t) noexcept { return sender.none; }
    void submit(any_none_receiver<E> to) { impl_->submit(std::move(to)); }
};


template <class Adapt, class From>
concept bool Adaptor =
    Sender<From> &&
    requires (Adapt const &a, From& from)
    {
        { a.adapt(from) } -> Sender;
    };

template <class Function>
class basic_adaptor
{
    Function f_;
public:
    basic_adaptor() = default;
    constexpr explicit basic_adaptor(Function f)
      : f_(std::move(f))
    {}
    template <Sender From>
      requires Invocable<Function&, From&> &&
               Sender<invoke_result_t<Function&, From&>>
    auto adapt(From& from) const
    {
        return f_(from);
    }
};
template <class Function>
basic_adaptor(Function) -> basic_adaptor<Function>;

} // namespace execution
} // inline namespace executors_v1
} // namespace experimental
} // namespace std

#endif // STD_EXPERIMENTAL_BITS_SENDER_RECEIVER_H
