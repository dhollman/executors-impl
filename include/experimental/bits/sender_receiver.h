#ifndef STD_EXPERIMENTAL_BITS_SENDER_RECEIVER_H
#define STD_EXPERIMENTAL_BITS_SENDER_RECEIVER_H

#include <exception>
#include <functional>
#include <type_traits>
#include <memory>
#include <future>

#include <experimental/bits/query.h>

namespace std {
// From C++20:
template <class A, class B>
concept bool Same = __is_same_as(A, B) && __is_same_as(B, A);

template <class A, class B>
concept bool _NotSame_ = !Same<A, B>;

template <class A, class B>
concept bool DerivedFrom = __is_base_of(B, A);

template <class From, class To>
concept bool ConvertibleTo =
  is_convertible_v<From, To> && requires (From(&from)()) { static_cast<To>(from()); };

template<class F, class... Args>
concept bool Invocable =
    requires(F&& f)
    {
        std::invoke((F&&) f, declval<Args>()...);
    };

template<class F, class... Args>
concept bool _VoidInvocable =
  Invocable<F, Args...> &&
  Same<void const volatile, invoke_result_t<F, Args...> const volatile>;

template<class F, class... Args>
concept bool _NonVoidInvocable =
  Invocable<F, Args...> && !_VoidInvocable<F, Args...>;

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

struct receiver_t : __property_base<false, false>
{
    template <class T>
    friend constexpr void query(promise<T>&, receiver_t)
    {}
};

template <class To>
concept bool _Receiver =
    Invocable<query_impl::query_fn const&, To&, receiver_t>;

inline constexpr struct set_done_fn
{
    template <class T>
    void operator()(std::promise<T>& to) const
        noexcept(noexcept(to.set_exception(make_exception_ptr(logic_error{""}))))
    {
        (void) to.set_exception(make_exception_ptr(
            logic_error{"std::promise doesn't support set_done"}));
    }
    template <_Receiver To>
      requires requires (To& to) { to.set_done(); }
    void operator()(To& to) const noexcept(noexcept(to.set_done()))
    {
        (void) to.set_done();
    }
    template <_Receiver To>
      requires requires (To& to) { set_done(to); }
    void operator()(To& to) const volatile noexcept(noexcept(set_done(to)))
    {
        (void) set_done(to);
    }
} const set_done {};

template <class To>
concept bool Receiver =
    _Receiver<To> && Invocable<set_done_fn const&, To&>;

inline constexpr struct set_value_fn
{
    // This works for std::promise<T> and std::promise<void> also:
    template <Receiver To, class... Args>
      requires requires (To& to, Args&&... args) { to.set_value((Args&&) args...); }
    void operator()(To& to, Args&&... args) const
        noexcept(noexcept(to.set_value((Args&&) args...)))
    {
        (void) to.set_value((Args&&) args...);
    }
    template <Receiver To, class... Args>
      requires requires (To& to, Args&&... args) { set_value(to, (Args&&) args...); }
    void operator()(To& to, Args&&... args) const
        noexcept(noexcept(set_value(to, (Args&&) args...)))
    {
        (void) set_value(to, (Args&&) args...);
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

template <class To, class E = exception_ptr, class... Args>
concept bool ReceiverOf =
    Receiver<To> &&
    Invocable<set_error_fn const&, To&, E> &&
    Invocable<set_value_fn const&, To&, Args...>;

namespace __test__
{
    struct S
    {
        static constexpr void query(receiver_t) noexcept
        {}
        void set_value(char const*) {}
    };
    void set_done(S) {}
    void set_error(S, int) {}

    static_assert((bool) Receiver<S>);
    static_assert((bool) ReceiverOf<S, int, char const*>);
} // namespace __test__

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
        { query_impl::query_fn{}(from, sender_t{}) } -> sender_desc<auto, auto...>;
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

template <_Sender From>
struct sender_traits : invoke_result_t<query_impl::query_fn const&, From&, sender_t>
{};

template <class E>
struct with_error
{
    using error_type = E;
};

template <class... Args>
struct with_values
{
    template <template <class...> class List>
    using value_types = List<Args...>;
};

template <class From>
concept bool Sender =
    _Sender<From> && Invocable<get_executor_fn const&, From&>;

inline constexpr struct submit_fn
{
    template <_Sender From, Receiver To>
      requires requires (From& from, To to) { from.submit((To&&) to); }
    void operator()(From& from, To to) const
        noexcept(noexcept(from.submit((To&&) to)))
    {
        (void) from.submit((To&&) to);
    }
    template <_Sender From, Receiver To>
      requires requires (From& from, To to) { submit(from, (To&&) to); }
    void operator()(From& from, To to) const volatile
        noexcept(noexcept(submit(from, (To&&) to)))
    {
        (void) submit(from, (To&&) to);
    }
} const submit {};

template <class From, class To>
concept bool SenderTo =
    Sender<From> && Receiver<To> && Invocable<submit_fn const&, From&, To>;

template <class To, class From>
concept bool ReceiverFrom = SenderTo<From, To>;

namespace __test__
{
    struct T
    {
        static constexpr sender_desc<int, char const*> query(sender_t) noexcept
        { return {}; }
        template <ReceiverOf<int, char const*> To>
        void submit(To to)
        {
            set_value(to, "hello world");
        }
    };
    static_assert((bool) Sender<T>);
    static_assert((bool) SenderTo<T, S>);
} // namespace __test__

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
  requires requires { typename OnSubmit::sender_desc_t; }
struct sender
{
private:
    [[no_unique_address]] OnSubmit on_submit_{};
    [[no_unique_address]] OnExecutor on_executor_{};
public:
    sender() = default;
    sender(OnSubmit on_submit) : on_submit_(std::move(on_submit))
    {}
    sender(OnSubmit on_submit, OnExecutor on_executor)
      : on_submit_(std::move(on_submit)), on_executor_(std::move(on_executor))
    {}
    static constexpr typename OnSubmit::sender_desc_t query(sender_t) noexcept
    {}
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
template <class OnSubmit>
sender(OnSubmit) -> sender<OnSubmit>;
template <class OnSubmit, class OnExecutor>
sender(OnSubmit, OnExecutor) -> sender<OnSubmit, OnExecutor>;

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

template <class...>
struct __typelist;

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
};

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



// template <class Adapt, class From>
// concept bool Adaptor =
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

inline constexpr struct __compose_fn
{
    template <class F, class G>
    struct __composed
    {
        [[no_unique_address]] F f_;
        [[no_unique_address]] G g_;
        template <class... Args>
          requires Invocable<G, Args...> &&
            Invocable<F, invoke_result_t<G, Args...>>
        constexpr decltype(auto) operator()(Args&&... args) &&
        {
            return std::invoke(std::move(f_), std::invoke(std::move(g_), (Args&&) args...));
        }
        template <class... Args>
          requires _VoidInvocable<G, Args...> && Invocable<F>
        constexpr decltype(auto) operator()(Args&&... args) &&
        {
            std::invoke(std::move(g_), (Args&&) args...);
            return std::move(f_)();
        }
        template <class... Args>
          requires Invocable<G&, Args...> &&
            Invocable<F&, invoke_result_t<G&, Args...>>
        constexpr decltype(auto) operator()(Args&&... args) &
        {
            return std::invoke(f_, std::invoke(g_, (Args&&) args...));
        }
        template <class... Args>
          requires _VoidInvocable<G&, Args...> && Invocable<F&>
        constexpr decltype(auto) operator()(Args&&... args) &
        {
            std::invoke(g_, (Args&&) args...);
            return f_();
        }
        template <class... Args>
          requires Invocable<G const&, Args...> &&
            Invocable<F const&, invoke_result_t<G const&, Args...>>
        constexpr decltype(auto) operator()(Args&&... args) const &
        {
            return std::invoke(f_, std::invoke(g_, (Args&&) args...));
        }
        template <class... Args>
          requires _VoidInvocable<G const&, Args...> && Invocable<F const&>
        constexpr decltype(auto) operator()(Args&&... args) const &
        {
            std::invoke(g_, (Args&&) args...);
            return f_();
        }
    };
    template <class F, class G>
    constexpr __composed<F, G> operator()(F f, G g) const
    {
        return __composed<F, G>{std::move(f), std::move(g)};
    }
} const __compose {};

template <class F, class G>
using __compose_result_t = decltype(__compose(declval<F>(), declval<G>()));

inline constexpr struct __bind1st_fn
{
    template <class T>
    static constexpr T __unwrap(T&&);
    template <class T>
    static constexpr T& __unwrap(reference_wrapper<T>);
    template <class T>
    using __unwrap_t = decltype(__unwrap(declval<T>()));
    template <class F, class T>
    struct __binder1st
    {
        [[no_unique_address]] F f_;
        [[no_unique_address]] T t_;
        template <class... Args>
          requires Invocable<F, T, Args...>
        constexpr decltype(auto) operator()(Args&&... args) &&
        {
            return std::invoke((F&&) f_, (T&&) t_, (Args&&) args...);
        }
        template <class... Args>
          requires Invocable<F&, T&, Args...>
        constexpr decltype(auto) operator()(Args&&... args) &
        {
            return std::invoke(f_, t_, (Args&&) args...);
        }
        template <class... Args>
          requires Invocable<F const&, T const&, Args...>
        constexpr decltype(auto) operator()(Args&&... args) const &
        {
            return std::invoke(f_, t_, (Args&&) args...);
        }
    };
    template <class F, class T>
    constexpr __binder1st<__unwrap_t<F>, __unwrap_t<T>> operator()(F f, T t) const
    {
        return __binder1st<__unwrap_t<F>, __unwrap_t<T>>{std::move(f), std::move(t)};
    }
} const __bind1st {};

template <class F, class T>
using __binder1st = __bind1st_fn::__binder1st<F, T>;
template <class F, class T>
using __bind1st_result_t = decltype(__bind1st(declval<F>(), declval<T>()));

template <template <class...> class T, class... Front>
struct __meta_bind_front
{
    template <class... Back>
    using __result = T<Front..., Back...>;
};

template <Sender From>
using sender_desc_t = invoke_result_t<query_impl::query_fn const&, From&, sender_t>;

template <Sender From, class Function>
using __transform_sender_desc_t =
    conditional_t<
        is_void_v<
            typename sender_desc_t<From>::template value_types<
                __meta_bind_front<invoke_result_t, Function>::template __result
            >
        >,
        sender_desc<typename sender_desc_t<From>::error_type>,
        sender_desc<
            typename sender_desc_t<From>::error_type,
            typename sender_desc_t<From>::template value_types<
                __meta_bind_front<invoke_result_t, Function>::template __result
            >
        >
    >;

} // namespace execution
} // inline namespace executors_v1
} // namespace experimental
} // namespace std

#endif // STD_EXPERIMENTAL_BITS_SENDER_RECEIVER_H
