
#pragma once

namespace std {
namespace experimental {
inline namespace executors_v1 {
namespace execution {

template <bool Requirable, bool Preferable>
struct __property_base
{
  static inline constexpr bool const is_requirable = Requirable;
  static inline constexpr bool const is_preferable = Preferable;
};
template <bool Requirable, bool Preferable>
constexpr bool const __property_base<Requirable, Preferable>::is_requirable;
template <bool Requirable, bool Preferable>
constexpr bool const __property_base<Requirable, Preferable>::is_preferable;

} // end namespace execution
} // end inline namespace executors_v1
} // end namespace experimental
} // end namespace std