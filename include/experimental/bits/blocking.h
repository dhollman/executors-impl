#ifndef STD_EXPERIMENTAL_BITS_BLOCKING_H
#define STD_EXPERIMENTAL_BITS_BLOCKING_H

#include <experimental/bits/blocking_adaptation.h>
#include <experimental/bits/helpers.h>
#include <experimental/bits/enumeration.h>
#include <experimental/bits/enumerator_adapter.h>
#include <future>
#include <any>

namespace std {
namespace experimental {
inline namespace executors_v1 {
namespace execution {

namespace blocking_t_impl {

template <typename Continuation, typename Promise>
auto _make_blocking_continuation(Continuation&& c, Promise&& p) {
  return execution::make_continuation(
    // Value handler
    [](auto&& val, auto&& subex, auto&& con, auto&& prom) {
      if constexpr (is_same_v<decay_t<decltype(val)>, monostate>) {
        auto rv = std::move(con).value(forward<decltype(subex)>(subex));
        std::move(prom).set_value();
        return rv;
      }
      else /* constexpr (not void continuation) */ {
        auto rv = std::move(con).value(forward<decltype(val)>(val), forward<decltype(subex)>(subex));
        std::move(prom).set_value();
        return rv;
      }
    },
    // Error handler
    [](auto&& err, auto&& subex, auto&& con, auto&& prom) {
      auto rv = std::move(con).error(forward<decltype(err)>(err), forward<decltype(subex)>(subex));
      std::move(prom).set_value();
      return rv;
    },
    // Args
    forward<Continuation>(c),
    forward<Promise>(p)
  );
}

struct any_then_value_executor {
  std::any value;


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

  public:
    using impl::enumerator_adapter<adapter, Executor, blocking_t, always_t>::enumerator_adapter;

    template<class Continuation>
    auto execute(Continuation&& c) &&
      -> decltype(inner_declval<Continuation>().execute(std::forward<Continuation>(c)))
    {
      _do_execute(std::move(this->executor_), forward<Continuation>(c));
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
