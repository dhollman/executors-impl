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

inline constexpr struct receiver_t
{
    static inline constexpr bool const is_requirable = false;
    static inline constexpr bool const is_preferable = false;
    static inline constexpr struct none_t {
        static inline constexpr size_t const cardinality = 0;
    } const none{};
    static inline constexpr struct single_t : none_t {
        static inline constexpr size_t const cardinality = 1;
    } const single{};
} const receiver {};

constexpr bool const receiver_t::is_requirable;
constexpr bool const receiver_t::is_preferable;
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
concept bool NoneReceiver =
    Receiver<To> &&
    requires (To& to, E&& e)
    {
        set_error(to, static_cast<E&&>(e));
    };

inline constexpr struct __set_value
{
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
} const set_value {};

template <class To, class V, class E = exception_ptr>
concept bool SingleReceiver =
    Receiver<To, receiver_t::single_t> &&
    NoneReceiver<To, E> &&
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
    static_assert((bool) NoneReceiver<S, int>);
    static_assert((bool) SingleReceiver<S, char const*, int>);
} // namespace __test__

inline constexpr struct sender_t
{
    static inline constexpr bool const is_requirable = false;
    static inline constexpr bool const is_preferable = false;
    static inline constexpr struct none_t {
        static inline constexpr size_t const cardinality = 0;
    } const none{};
    static inline constexpr struct single_t : none_t {
        static inline constexpr size_t const cardinality = 1;
    } const single{};
} const sender {};

constexpr bool const sender_t::is_requirable;
constexpr bool const sender_t::is_preferable;
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
    template <Sender From, Receiver To>
      requires requires (From& from, To to) { from.submit((To&&) to); } &&
        sender_category_t<From>::cardinality <= receiver_category_t<To>::cardinality
    void operator()(From& from, To to) const noexcept(noexcept(from.submit((To&&) to)))
    {
        from.submit((To&&) to);
    }
    template <Sender From, Receiver To>
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
{
};
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
    template <class V>
    void operator()(V&&) const noexcept
    {}
} const ignore_value{};

template <class OnDone, class OnError, class OnValue = __nope>
  requires Invocable<OnDone&>
struct basic_receiver
{
private:
    [[no_unique_address]] OnDone on_done_{};
    [[no_unique_address]] OnError on_error_{};
    [[no_unique_address]] OnValue on_value_{};
public:
    static constexpr auto query(receiver_t)
    {
        if constexpr (is_same_v<OnValue, __nope>)
            return receiver.none;
        else
            return receiver.single;
    }
    basic_receiver() = default;
    basic_receiver(OnValue on_value) : on_value_(std::move(on_value))
    {}
    basic_receiver(OnValue on_value, OnError on_error)
      : on_error_(std::move(on_error)), on_value_(std::move(on_value))
    {}
    basic_receiver(OnValue on_value, OnError on_error, OnDone on_done)
      : on_done_(std::move(on_done)), on_error_(std::move(on_error)),
        on_value_(std::move(on_value))
    {}
    basic_receiver(OnError on_error)
      : on_error_(std::move(on_error))
    {}
    basic_receiver(OnError on_error, OnDone on_done)
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
    template <class V>
      requires Invocable<OnValue&, V>
    void set_value(V&& v)
    {
        on_value_((V&&) v);
    }
};

basic_receiver() -> basic_receiver<__ignore_done, __ignore_error, __ignore_value>;
template<class OnValue>
basic_receiver(OnValue) -> basic_receiver<__ignore_done, __ignore_error, OnValue>;
template<class OnValue, class OnError>
basic_receiver(OnValue, OnError) -> basic_receiver<__ignore_done, OnError, OnValue>;
template<class OnValue, class OnError, class OnDone>
basic_receiver(OnValue, OnError, OnDone) -> basic_receiver<OnDone, OnError, OnValue>;
template<class OnError, class OnDone>
  requires Invocable<OnDone&>
basic_receiver(OnError, OnDone) -> basic_receiver<OnDone, OnError>;

} // namespace execution
} // inline namespace executors_v1
} // namespace experimental
} // namespace std

#endif // STD_EXPERIMENTAL_BITS_SENDER_RECEIVER_H
