#ifndef STD_EXPERIMENTAL_BITS_BULK_GUARANTEE_H
#define STD_EXPERIMENTAL_BITS_BULK_GUARANTEE_H

#include <experimental/bits/enumeration.h>
#include <experimental/bits/enumerator_adapter.h>

namespace std {
namespace experimental {
inline namespace executors_v1 {
namespace execution {

struct bulk_guarantee_t :
  impl::enumeration<bulk_guarantee_t, 3>
{
  using impl::enumeration<bulk_guarantee_t, 3>::enumeration;

  using unsequenced_t = enumerator<0>;
  using sequenced_t = enumerator<1>;
  using parallel_t = enumerator<2>;

  static constexpr unsequenced_t unsequenced{};
  static constexpr sequenced_t sequenced{};
  static constexpr parallel_t parallel{};

private:

  template <class Executor, class Enumerator> class adapter;

  template <class Enumerator>
  struct adapter_for_enumerator_mfc {
    template <class Executor>
    using apply = adapter<Executor, Enumerator>;
  };

  template <class Executor, class Enumerator>
  class adapter :
    public impl::trivial_adapter_mixin<Executor,
      impl::enumerator_adapter<
        adapter_for_enumerator_mfc<Enumerator>::template apply, Executor, bulk_guarantee_t, Enumerator
      >
    >
  {
  private:
    using base_t = impl::trivial_adapter_mixin<Executor,
      impl::enumerator_adapter<
        adapter_for_enumerator_mfc<Enumerator>::template apply, Executor, bulk_guarantee_t, Enumerator
      >
    >;
  public:

    using base_t::base_t;
    using base_t::execute;
    using base_t::then_execute;

  };

public:

  template <typename Executor>
  friend adapter<Executor, sequenced_t> require(Executor ex, sequenced_t)
  {
    return adapter<Executor, sequenced_t>(std::move(ex));
  }
  template <typename Executor>
  friend adapter<Executor, parallel_t> require(Executor ex, parallel_t)
  {
    return adapter<Executor, parallel_t>(std::move(ex));
  }
};

constexpr bulk_guarantee_t bulk_guarantee{};
inline constexpr bulk_guarantee_t::unsequenced_t bulk_guarantee_t::unsequenced;
inline constexpr bulk_guarantee_t::sequenced_t bulk_guarantee_t::sequenced;
inline constexpr bulk_guarantee_t::parallel_t bulk_guarantee_t::parallel;

} // namespace execution
} // inline namespace executors_v1
} // namespace experimental
} // namespace std

#endif // STD_EXPERIMENTAL_BITS_BULK_GUARANTEE_H
