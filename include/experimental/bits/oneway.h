#ifndef STD_EXPERIMENTAL_BITS_ONEWAY_H
#define STD_EXPERIMENTAL_BITS_ONEWAY_H

#include <experimental/bits/is_oneway_executor.h>

namespace std {
namespace experimental {
inline namespace executors_v1 {
namespace execution {

struct oneway_t
{
  static constexpr bool is_requirable = true;
  static constexpr bool is_preferable = false;

  template<class... SupportableProperties>
    class polymorphic_executor_type;

  using polymorphic_query_result_type = bool;
  static constexpr bool value() { return true; }
};

constexpr oneway_t oneway;

} // namespace execution
} // inline namespace executors_v1
} // namespace experimental
} // namespace std

#endif // STD_EXPERIMENTAL_BITS_ONEWAY_H
