#ifndef STD_EXPERIMENTAL_BITS_SENDER_RECEIVER_H
#define STD_EXPERIMENTAL_BITS_SENDER_RECEIVER_H

#include <experimental/bits/query.h>

namespace std {
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
concept bool Receiver =
    requires (To& to, Tag t)
    {
        { t } -> receiver_t::none_t;
        { Tag::cardinality } -> size_t;
        { query_impl::customization_point<>(to, receiver) } -> Tag;
    } &&
    Tag::cardinality <= receiver.single.cardinality;

template <Receiver To>
using receiver_category_t =
    decltype(query_impl::customization_point<>(std::declval<To&>(), receiver));

inline constexpr struct __set_error
{
    template <Receiver To, class E>
    auto operator()(To& to, E&& e) const noexcept(noexcept(set_error(to, (E&&) e)))
        -> decltype(set_error(to, (E&&) e), void())
    {
        set_error(to, (E&&) e);
    }
    template <Receiver To, class E>
    auto operator()(To& to, E&& e) const noexcept(noexcept(to.set_error((E&&) e)))
        -> decltype(to.set_error((E&&) e), void()) requires true
    {
        to.set_error((E&&) e);
    }
} const set_error {};

inline constexpr struct __set_value
{
    template <Receiver<receiver_t::single_t> To, class V>
    auto operator()(To& to, V&& v) const noexcept(noexcept(set_value(to, (V&&) v)))
        -> decltype(set_value(to, (V&&) v), void())
    {
        set_value(to, (V&&) v);
    }
    template <Receiver<receiver_t::single_t> To, class V>
    auto operator()(To& to, V&& v) const noexcept(noexcept(to.set_value((V&&) v)))
        -> decltype(to.set_value((V&&) v), void()) requires true
    {
        to.set_value((V&&) v);
    }
} const set_value {};

template <class To, class E>
concept bool NoneReceiver =
    Receiver<To> &&
    requires (To& to, E&& e)
    {
        set_error(to, static_cast<E&&>(e));
    };

template <class To, class E, class V>
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
    void set_error(S, int) {}

    static_assert((bool) Receiver<S>);
    static_assert((bool) NoneReceiver<S, int>);
    static_assert((bool) SingleReceiver<S, int, char const*>);
}

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
concept bool Sender =
    requires (From& from, Tag t)
    {
        { t } -> sender_t::none_t;
        { Tag::cardinality } -> size_t;
        { query_impl::customization_point<>(from, sender) } -> Tag;
    } &&
    Tag::cardinality <= sender.single.cardinality;

template <Sender From>
using sender_category_t =
    decltype(query_impl::customization_point<>(std::declval<From&>(), sender));

inline constexpr struct __submit
{
    template <Sender From, Receiver To>
      requires sender_category_t<From>::cardinality <= receiver_category_t<To>::cardinality
    auto operator()(From& from, To to) const noexcept(noexcept(submit(from, (To&&) to)))
        -> decltype(submit(from, (To&&) to), void())
    {
        submit(from, (To&&) to);
    }
    template <Sender From, Receiver To>
      requires sender_category_t<From>::cardinality <= receiver_category_t<To>::cardinality
    auto operator()(From& from, To to) const noexcept(noexcept(from.submit((To&&) to)))
        -> decltype(from.submit((To&&) to), void()) requires true
    {
        from.submit((To&&) to);
    }
} const submit {};

template <class From, class To>
concept bool SenderTo =
    Sender<From> && Receiver<To> &&
    requires (From& from, To&& to)
    {
        submit(from, (To&&) to);
    };

namespace __test__
{
    struct T
    {
        static constexpr auto query(sender_t) noexcept
        {
            return sender.single;
        }
        template <SingleReceiver<int, char const*> To>
        void submit(To to)
        {
            set_value(to, "hello world");
        }
    };
    static_assert(Sender<T>);
    static_assert(SenderTo<T, S>);
}

} // namespace execution
} // inline namespace executors_v1
} // namespace experimental
} // namespace std

#endif // STD_EXPERIMENTAL_BITS_SENDER_RECEIVER_H
