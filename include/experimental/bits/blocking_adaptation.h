#ifndef STD_EXPERIMENTAL_BITS_BLOCKING_ADAPTATION_H
#define STD_EXPERIMENTAL_BITS_BLOCKING_ADAPTATION_H

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

    template<class Function> auto execute(Function f) const
      -> decltype(inner_declval<Function>().execute(std::move(f)))
    {
      return this->executor_.execute(std::move(f));
    }

    template <Sender From, class Function>
      requires Invocable<Function&>
    struct __oneway_sender
    {
      From from_;
      Function f_;
      adapter this_;
      static constexpr auto query(sender_t)
      {
        return sender.none;
      }
      template <NoneReceiver To>
      void submit(To to)
      {
        struct __receiver
        {
          Function f_;
          To to_;
          Executor exec_;
          static constexpr auto query(execution::receiver_t)
          {
            return execution::receiver.none;
          }
          void set_error(std::exception_ptr e)
          {
            execution::set_error(to_, e);
          }
          void set_done()
          {
            execution::set_done(to_);
          }
          void set_value()
          {
            exec_.submit(
              single_receiver{
                [f = std::move(f_), to = std::move(to_)](auto) mutable
                {
                  f();
                  execution::set_value(to);
                }
              }
            );
          }
        };
        execution::submit(
          from_,
          __receiver{std::move(f_), std::move(to), this_.executor}
        );
      }
      auto executor() const
      {
        return this_;
      }
    };
    template<Sender From, class Function>
      requires Invocable<Function&>
    auto make_value_task(From from, Function f) const -> Sender
    {
      return __oneway_sender<From, Function>{std::move(from), std::move(f), *this};
    }

    template<class Function>
    auto twoway_execute(Function f) const
      -> decltype(inner_declval<Function>().twoway_execute(std::move(f)))
    {
      return this->executor_.twoway_execute(std::move(f));
    }

    // template <class Function>
    //   requires _NonVoidInvocable<Function&>
    // struct __twoway_sender
    // {
    //   Function f_;
    //   adapter this_;
    //   static constexpr auto query(sender_t)
    //   {
    //     return sender.single;
    //   }
    //   template <SingleReceiver<invoke_result_t<Function&>> To>
    //   void submit(To to)
    //   {
    //     this_.executor.submit(
    //       single_receiver{
    //         [f = std::move(f_), to = std::move(to)](Executor) mutable
    //         {
    //           set_value(to, f());
    //         }
    //       }
    //     );
    //   }
    //   auto executor() const
    //   {
    //     return this_;
    //   }
    // };
    // template<class Function>
    //   requires _NonVoidInvocable<Function&>
    // auto twoway_execute_(Function f) const -> Sender<sender_t::single_t>
    // {
    //   return __twoway_sender<Function>{std::move(f), *this};
    // }

    template<class Function, class SharedFactory>
    auto bulk_execute(Function f, std::size_t n, SharedFactory sf) const
      -> decltype(inner_declval<Function>().bulk_execute(std::move(f), n, std::move(sf)))
    {
      return this->executor_.bulk_execute(std::move(f), n, std::move(sf));
    }

    template<class Function, class ResultFactory, class SharedFactory>
    auto bulk_twoway_execute(Function f, std::size_t n, ResultFactory rf, SharedFactory sf) const
      -> decltype(inner_declval<Function>().bulk_twoway_execute(std::move(f), n, std::move(rf), std::move(sf)))
    {
      return this->executor_.bulk_twoway_execute(std::move(f), n, std::move(rf), std::move(sf));
    }

    template<SingleReceiver<adapter&> To>
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
