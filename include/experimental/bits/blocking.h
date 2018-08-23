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
    template <class T> static auto inner_declval() -> decltype(std::declval<Executor>());

  public:
    using impl::enumerator_adapter<adapter, Executor, blocking_t, always_t>::enumerator_adapter;

    template<class Function> auto execute(Function f) const
      -> decltype(inner_declval<Function>().execute(std::move(f)))
    {
      promise<void> promise;
      future<void> future = promise.get_future();
      this->executor_.execute([f = std::move(f), p = std::move(promise)]() mutable { f(); });
      future.wait();
    }

    template <Sender From, class Function>
      requires Invocable<Function&>
    struct __oneway_sender
    {
      From from_;
      Function f_;
      adapter this_;
      static constexpr void query(sender_t)
      {}
      template <ReceiverOf<exception_ptr> To>
      void submit(To to)
      {
        struct __receiver
        {
          std::promise<void> p_;
          Function f_;
          To to_;
          Executor exec_;
          static constexpr void query(execution::receiver_t)
          {}
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
              receiver{
                on_value{
                  [p = std::move(p_), f = std::move(f_)](Executor) mutable
                  {
                    f();
                    p.set_value();
                  }
                }
              }
            );
          }
        };
        std::promise<void> p;
        auto fut = p.get_future();
        from_.submit(
          __receiver{std::move(p), std::move(f_), std::move(to), this_.executor_}
        );
        fut.wait();
        set_value(to);
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
      auto future = this->executor_.twoway_execute(std::move(f));
      future.wait();
      return future;
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
    //     promise<invoke_result_t<Function&>> p;
    //     auto f = p.get_future();
    //     this_.executor_.submit(
    //       single_receiver{
    //         [p = std::move(p), f = std::move(f_)](Executor) mutable
    //         {
    //           p.set_value(f());
    //         }
    //       }
    //     );
    //     set_value(to, f.get());
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
    //   return __twoway_sender<Function>{std::move(f), *this, std::move(submit)};
    // }

    template<class Function, class SharedFactory>
    auto bulk_execute(Function f, std::size_t n, SharedFactory sf) const
      -> decltype(inner_declval<Function>().bulk_execute(std::move(f), n, std::move(sf)))
    {
      promise<void> promise;
      future<void> future = promise.get_future();
      this->executor_.bulk_execute([f = std::move(f), p = std::move(promise)](auto i, auto& s) mutable { f(i, s); }, n, std::move(sf));
      future.wait();
    }

    template<class Function, class ResultFactory, class SharedFactory>
    auto bulk_twoway_execute(Function f, std::size_t n, ResultFactory rf, SharedFactory sf) const
      -> decltype(inner_declval<Function>().bulk_twoway_execute(std::move(f), n, std::move(rf), std::move(sf)))
    {
      auto future = this->executor_.bulk_twoway_execute(std::move(f), n, std::move(rf), std::move(sf));
      future.wait();
      return future;
    }

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
