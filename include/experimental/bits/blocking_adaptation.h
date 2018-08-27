#ifndef STD_EXPERIMENTAL_BITS_BLOCKING_ADAPTATION_H
#define STD_EXPERIMENTAL_BITS_BLOCKING_ADAPTATION_H

#include <tuple>

#include <experimental/bits/enumeration.h>
#include <experimental/bits/enumerator_adapter.h>
#include <experimental/bits/sender_receiver.h>

namespace std {
namespace experimental {
inline namespace executors_v1 {
namespace execution {

struct blocking_adaptation_t :
  impl::enumeration<blocking_adaptation_t, 2>
{
  using impl::enumeration<blocking_adaptation_t, 2>::enumeration;

  using disallowed_t = enumerator<0>;
  using allowed_t = enumerator<1>;

  static constexpr disallowed_t disallowed{};
  static constexpr allowed_t allowed{};

private:
  template <typename Executor>
  class adapter :
    public impl::enumerator_adapter<adapter,
      Executor, blocking_adaptation_t, allowed_t>
  {
    template <class T> static auto inner_declval() -> decltype(std::declval<Executor>());

  public:
    using impl::enumerator_adapter<adapter, Executor,
      blocking_adaptation_t, allowed_t>::enumerator_adapter;

    // template<class Function> auto execute(Function f) const
    //   -> decltype(inner_declval<Function>().execute(std::move(f)))
    // {
    //   return this->executor_.execute(std::move(f));
    // }

    template <Sender From, class Function>
    struct __sender
    {
      From from_;
      Function f_;
      adapter this_;
      static constexpr execution::__transform_sender_desc_t<From, Function&> query(sender_t)
      { return {}; }
      template <Receiver To>
      struct __receiver
      {
        Function f_;
        To to_;
        Executor exec_;
        using __composed_fn =
          __compose_result_t<
            __binder1st<execution::set_value_fn, To&>,
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
        template <class... Ts>
          requires Invocable<__composed_fn, Ts...>
        void set_value(Ts&&... ts)
        {
          execution::submit(
            exec_,
            receiver{
              on_value{
                // make_tuple? Or forward lvalue references?
                [f = std::move(f_), to = std::move(to_), args = std::make_tuple((Ts&&) ts...)](auto) mutable
                {
                  // Roughly: set_value(to, f(ts...))
                  std::apply(
                      __compose(__bind1st(execution::set_value, std::ref(to)), std::move(f)),
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
        execution::submit(
          from_,
          __receiver<To>{std::move(f_), std::move(to), this_.executor}
        );
      }
      auto executor() const
      {
        return this_;
      }
    };
    template<Sender From, class Function>
    auto make_value_task(From from, Function f) const -> Sender
    {
      return __sender<From, Function>{std::move(from), std::move(f), *this};
    }

    // template<class Function>
    // auto twoway_execute(Function f) const
    //   -> decltype(inner_declval<Function>().twoway_execute(std::move(f)))
    // {
    //   return this->executor_.twoway_execute(std::move(f));
    // }

    // template<class Function, class SharedFactory>
    // auto bulk_execute(Function f, std::size_t n, SharedFactory sf) const
    //   -> decltype(inner_declval<Function>().bulk_execute(std::move(f), n, std::move(sf)))
    // {
    //   return this->executor_.bulk_execute(std::move(f), n, std::move(sf));
    // }

    // template<class Function, class ResultFactory, class SharedFactory>
    // auto bulk_twoway_execute(Function f, std::size_t n, ResultFactory rf, SharedFactory sf) const
    //   -> decltype(inner_declval<Function>().bulk_twoway_execute(std::move(f), n, std::move(rf), std::move(sf)))
    // {
    //   return this->executor_.bulk_twoway_execute(std::move(f), n, std::move(rf), std::move(sf));
    // }

    template<ReceiverOf<exception_ptr, adapter&> To>
    void submit(To to)
    {
      set_value((To&&) to, *this);
    }
  };

public:
  template <typename Executor>
  friend adapter<Executor> require(Executor ex, allowed_t)
  {
    return adapter<Executor>(std::move(ex));
  }
};

constexpr blocking_adaptation_t blocking_adaptation{};
inline constexpr blocking_adaptation_t::disallowed_t blocking_adaptation_t::disallowed;
inline constexpr blocking_adaptation_t::allowed_t blocking_adaptation_t::allowed;

} // namespace execution
} // inline namespace executors_v1
} // namespace experimental
} // namespace std

#endif // STD_EXPERIMENTAL_BITS_BLOCKING_ADAPTATION_H
