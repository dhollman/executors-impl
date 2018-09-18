
#pragma once

namespace std {
namespace experimental {
inline namespace executors_v1 {
namespace execution {

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

template <Sender From, class Function>
using __values_transform_result_t =
    typename sender_traits<From>::template value_types<
        __meta_bind_front<invoke_result_t, Function>::template __result
    >;

template <class From, class Function>
_CONCEPT TransformedSender =
    Sender<From> && requires
    {
        typename __values_transform_result_t<From, Function>;
    };

template <Sender From, class Function>
  requires TransformedSender<From, Function>
using __transform_sender_desc_t =
    conditional_t<
        is_void_v<__values_transform_result_t<From, Function>>,
        sender_desc<typename sender_traits<From>::error_type>,
        sender_desc<
            typename sender_traits<From>::error_type,
            __values_transform_result_t<From, Function>
        >
    >;

template <Sender From, class Function>
  requires TransformedSender<From, Function&>
struct transform_sender
{
private:
    From from_;
    Function fun_;
    template <Receiver To>
    struct __receiver
    {
        To to_;
        Function fun_;
        using __composed_fn =
            __compose_result_t<__binder1st<set_value_fn, To&>, Function>;
        static constexpr void query(receiver_t) noexcept {}
        void set_done() { execution::set_done(to_); }
        template <class E>
          requires Invocable<execution::set_error_fn const&, To&, E>
        void set_error(E&& e)
        {
            execution::set_error(to_, (E&&) e);
        }
        template <class...Args>
          requires Invocable<__composed_fn, Args...>
        void set_value(Args&&... args)
        {
            __compose(
                __bind1st(execution::set_value, std::ref(to_)),
                std::move(fun_)
            )((Args&&) args...);
        }
    };
public:
    transform_sender() = default;
    transform_sender(From from, Function fun)
      : from_(std::move(from)), fun_(std::move(fun))
    {}
    static constexpr __transform_sender_desc_t<From, Function&> query(sender_t) noexcept
    { return {}; }
    template <Receiver To>
      requires SenderTo<From, __receiver<To>>
    void submit(To to)
    {
        execution::submit(
            from_,
            __receiver<To>{std::move(to), std::move(fun_)}
        );
    }
};

} // namespace execution
} // inline namespace executors_v1
} // namespace experimental
} // namespace std
