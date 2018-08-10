#ifndef STD_EXPERIMENTAL_BITS_BLOCKING_H
#define STD_EXPERIMENTAL_BITS_BLOCKING_H

#include <experimental/bits/blocking_adaptation.h>
#include <experimental/bits/helpers.h>
#include <experimental/bits/enumeration.h>
#include <experimental/bits/enumerator_adapter.h>

#include <experimental/execution>

#include <future>
#include <cassert>

namespace std {
namespace experimental {
inline namespace executors_v1 {
namespace execution {

namespace blocking_t_impl {

template <typename Continuation, typename Promise>
auto _make_blocking_continuation(Continuation&& c, Promise&& p) {
  return execution::make_continuation(
    // Value handler
    [](auto&& val, auto&& con, auto&& prom) {
      if constexpr (is_same_v<decay_t<decltype(val)>, monostate>) {
        auto rv = std::move(con).value();
        std::move(prom).set_value();
        return rv;
      }
      else /* constexpr (not void continuation) */ {
        auto rv = std::move(con).value(forward<decltype(val)>(val));
        std::move(prom).set_value();
        return rv;
      }
    },
    // Error handler
    [](auto&& err, auto&& con, auto&& prom) {
      auto rv = std::move(con).error(forward<decltype(err)>(err));
      std::move(prom).set_value();
      return rv;
    },
    // Args
    forward<Continuation>(c),
    forward<Promise>(p)
  );
}

template <class ValueExecutor, class ErrorExecutor>
struct variant_executor_adapter {
  variant<ValueExecutor, ErrorExecutor> ex_;

  using value_type = executor_value_t<ValueExecutor>;
  using error_type = executor_error_t<ErrorExecutor>;

  template <typename Property>
  auto require(const Property& p) &&
    // TODO noexcept specification
    -> variant_executor_adapter<
         decltype(execution::require(declval<ValueExecutor>, declval<Property>)),
         decltype(execution::require(declval<ErrorExecutor>, declval<Property>))
       >
  {
    if(auto* ex = std::get_if<ValueExecutor>(&ex_)) {
      return execution::require(std::move(*ex), p);
    }
    else {
      auto* err_ex = std::get_if<ErrorExecutor>(&ex_);
      assert(err_ex);
      return execution::require(std::move(*err_ex), p);
    }
  }

  template <typename Property>
  constexpr auto query(const Property& p) const
    // TODO noexcept specification
    -> enable_if_t<
         is_same_v<
           decltype(execution::query(declval<ValueExecutor const&>, declval<Property>)),
           decltype(execution::query(declval<ErrorExecutor const&>, declval<Property>))
         >,
         decltype(execution::query(declval<ValueExecutor>, declval<Property>))
       >
  {
    if(auto* ex = std::get_if<ValueExecutor>(&ex_)) {
      return execution::query(*ex, p);
    }
    else {
      auto* err_ex = std::get_if<ErrorExecutor>(&ex_);
      assert(err_ex);
      return execution::query(*err_ex, p);
    }
  }

  template <typename Continuation>
  void execute(Continuation&& c) && {
    if(auto* ex = std::get_if<ValueExecutor>(&ex_)) {
      std::move(*ex).execute(forward<Continuation>(c));
    }
    else {
      auto* err_ex = std::get_if<ErrorExecutor>(&ex_);
      assert(ex);
      std::move(*err_ex).execute(forward<Continuation>(c));
    }
  }
};


} // end namespace blocking_t_impl

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

    template <typename ExecutorDeduced, typename Continuation>
    static auto _do_execute(ExecutorDeduced&& ex, Continuation&& c) {
      promise<void> promise;
      future<void> future = promise.get_future();

      std::forward<ExecutorDeduced>(ex).execute(
        blocking_t_impl::_make_blocking_continuation(forward<Continuation>(c), move(promise))
      );
      future.wait();
    }


    template <typename ExecutorDeduced, typename Continuation>
    static auto _do_then_execute(ExecutorDeduced&& ex, Continuation&& c) {
      promise<void> prom;
      future<void> future = prom.get_future();

      using value_t = executor_value_t<Executor>;
      using error_t = executor_error_t<Executor>;

      auto return_factory = execution::query(ex, executor_factory<value_t, error_t>);

      using value_executor_t = decay_t<decltype(return_factory.make_executor(declval<value_t>()))>;
      using error_executor_t = decay_t<decltype(return_factory.make_error_executor(declval<error_t>()))>;

      optional<blocking_t_impl::variant_executor_adapter<value_executor_t, error_executor_t>> rv;

      std::forward<ExecutorDeduced>(ex).then_execute(
        forward<Continuation>(c)
      ).execute(execution::make_continuation(
        [&](auto&& v) {
          if constexpr (!is_same_v<decay_t<decltype(v)>, monostate>) {
            rv = blocking_t_impl::variant_executor_adapter<value_executor_t, error_executor_t>{
              return_factory.make_executor(forward<decltype(v)>(v))
            };
          }
          else { /* constexpr value_t is void */
            rv = blocking_t_impl::variant_executor_adapter<value_executor_t, error_executor_t>{
              return_factory.make_executor()
            };
          }
          prom.set_value();
        },
        [&](auto&& e) {
          rv = blocking_t_impl::variant_executor_adapter<value_executor_t, error_executor_t>{
            return_factory.make_error_executor(forward<decltype(e)>(e))
          };
          prom.set_value();
        }
      ));
      future.wait();
      return *rv;
    }

  public:
    using impl::enumerator_adapter<adapter, Executor, blocking_t, always_t>::enumerator_adapter;

    template<class Continuation>
    auto execute(Continuation&& c) &&
      -> decltype(inner_declval<Continuation>().execute(std::forward<Continuation>(c)))
    {
      _do_execute(std::move(this->executor_), forward<Continuation>(c));
    }

    // TODO Constrain to executors that can call then_execute
    template<class Continuation,
      class=enable_if_t<can_require_v<Executor, then_t>>
    >
    auto then_execute(Continuation&& c) &&
    {
      return _do_then_execute(execution::require(std::move(this->executor_), then), forward<Continuation>(c));
    }
  };

  template <class Executor>
  class possibly_adapter :
    public impl::trivial_adapter_mixin<Executor,
      impl::enumerator_adapter<possibly_adapter, Executor, blocking_t, possibly_t>
    >
  {
    private:

      using base_t = impl::trivial_adapter_mixin<Executor,
        impl::enumerator_adapter<possibly_adapter, Executor, blocking_t, possibly_t>
      >;

    public:

      using base_t::base_t;

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
  // anything can be adapted to blocking.possibly, since it means any kind of blocking:
  template<class Executor>
  friend possibly_adapter<Executor> require(Executor ex, possibly_t)
  {
    return possibly_adapter<Executor>(std::move(ex));
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
