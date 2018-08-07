#ifndef STD_EXPERIMENTAL_BITS_BLOCKING_ADAPTATION_H
#define STD_EXPERIMENTAL_BITS_BLOCKING_ADAPTATION_H

#include <experimental/bits/enumeration.h>
#include <experimental/bits/enumerator_adapter.h>

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
    public impl::trivial_adapter_mixin<
      Executor,
      impl::enumerator_adapter<adapter, Executor, blocking_adaptation_t, allowed_t>
    >
  {
    using base_t =
      impl::trivial_adapter_mixin<
        Executor,
        impl::enumerator_adapter<adapter, Executor, blocking_adaptation_t, allowed_t>
      >;

    template <typename ExecutorDeduced, typename OldExecutor>
    adapter(ExecutorDeduced&& ex, adapter<OldExecutor>&& old)
      : base_t(std::forward<ExecutorDeduced>(ex))
    { }

    template <template <class> class, class>
    friend class impl::adapter;

  public:

    using base_t::base_t;

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
