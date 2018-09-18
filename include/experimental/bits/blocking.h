#ifndef STD_EXPERIMENTAL_BITS_BLOCKING_H
#define STD_EXPERIMENTAL_BITS_BLOCKING_H

#include <experimental/bits/blocking_adaptation.h>
#include <experimental/bits/enumeration.h>
#include <experimental/bits/enumerator_adapter.h>
#include <future>

namespace std {
namespace experimental {
inline namespace executors_v1 {
namespace execution {

struct blocking_t :
  impl::enumeration<blocking_t, 3>
{
  using impl::enumeration<blocking_t, 3>::enumeration;

  using possibly_t = enumerator<0>;
  using always_t = enumerator<1>;
  using never_t = enumerator<2>;

  static constexpr possibly_t possibly{};
  static constexpr always_t always{};
  static constexpr never_t never{};

private:
  template<class Executor>
  class adapter :
    public impl::enumerator_adapter<adapter, Executor, blocking_t, always_t>
  {
    // template <class T> static auto inner_declval() -> decltype(std::declval<Executor>());

    template <Sender From, class Function, class Value>
    struct __sender
    {
      From from_;
      Function f_;
      adapter this_;
      static constexpr __transform_sender_desc_t<From, Function&> query(sender_t)
      { return {}; }
      template <Receiver To>
      struct __receiver
      {
        std::promise<Value> p_;
        Function f_;
        To& to_;
        Executor exec_;
        using __composed_fn =
          __compose_result_t<
            __binder1st<execution::set_value_fn, std::promise<Value>&>,
            Function&>;
        static constexpr void query(execution::receiver_t)
        {}
        template <class E>
          requires Invocable<execution::set_error_fn const&, To&, E>
        void set_error(E&& e)
        {
          execution::set_error(to_, (E&&) e);
        }
        void set_done()
        {
          execution::set_done(to_);
        }
        template <class... Args>
          requires Invocable<__composed_fn, Args...>
        void set_value(Args&&... args)
        {
          execution::submit(
            exec_,
            receiver{
              on_value{
                [p = std::move(p_), f = std::move(f_), args = std::make_tuple((Args&&) args...)](Executor) mutable
                {
                  // Roughly: p.set_value(f(ts...))
                  std::apply(
                      __compose(__bind1st(execution::set_value, std::ref(p)), std::move(f)),
                      std::move(args)
                  );
                }
              }
            }
          );
        }
      };
      template <Receiver To>
        requires SenderTo<From, __receiver<To>>
      void submit(To to)
      {
        std::promise<Value> p;
        std::future<Value> fut = p.get_future();
        execution::submit(
          from_,
          __receiver<To>{std::move(p), std::move(f_), to, this_.executor_}
        );
        // Roughly: `set_value(to, fut.get())`, or `fut.get(); set_value(to)`
        // if `Value` is `void`:
        __compose(
          __bind1st(execution::set_value, std::ref(to)),
          &std::future<Value>::get
        )(fut);
      }
      auto executor() const
      {
        return this_;
      }
    };
  public:
    using impl::enumerator_adapter<adapter, Executor, blocking_t, always_t>::enumerator_adapter;

    template<class Function, TransformedSender<Function&> From>
    auto make_value_task(From from, Function f) const
    {
      // If `From` sends arguments `Args...` through the value channel, then
      // `value_t` is `invoke_result_t<Function&, Args...>`:
      using value_t = __values_transform_result_t<From, Function&>;
      return __sender<From, Function, value_t>{std::move(from), std::move(f), *this};
    }

    // BUGBUG TODO
    // void make_bulk_value_task(...)

    template <ReceiverOf<exception_ptr, adapter&> To>
    void submit(To to)
    {
      set_value((To&&) to, *this);
    }
  };

public:
  template<class Executor,
    typename = std::enable_if_t<
      blocking_adaptation_t::static_query_v<Executor>
        == blocking_adaptation.allowed
    >>
  friend adapter<Executor> require(Executor ex, always_t)
  {
    return adapter<Executor>(std::move(ex));
  }
};

constexpr blocking_t blocking{};
inline constexpr blocking_t::possibly_t blocking_t::possibly;
inline constexpr blocking_t::always_t blocking_t::always;
inline constexpr blocking_t::never_t blocking_t::never;

} // namespace execution
} // inline namespace executors_v1
} // namespace experimental
} // namespace std

#endif // STD_EXPERIMENTAL_BITS_BLOCKING_H
