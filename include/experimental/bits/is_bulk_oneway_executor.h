#ifndef STD_EXPERIMENTAL_BITS_IS_BULK_ONEWAY_EXECUTOR_H
#define STD_EXPERIMENTAL_BITS_IS_BULK_ONEWAY_EXECUTOR_H

#include <experimental/bits/is_executor.h>
#include <experimental/bits/require.h>
#include <experimental/bits/sender_receiver.h>// BUGBUG for Invocable

namespace std {
namespace experimental {
inline namespace executors_v1 {
namespace execution {
namespace is_bulk_oneway_executor_impl {

struct shared_state {};

struct shared_factory
{
  shared_state operator()() const { return {}; }
};

struct bulk_function
{
  void operator()(std::size_t, shared_state&) {}
};

template<class T>
struct eval : std::false_type {};

template<class T>
  requires Same<T, invoke_result_t<require_impl::require_fn, T&, bulk_oneway_t>>
struct eval<T> : is_executor_impl::eval<T> {};

} // namespace is_bulk_oneway_executor_impl

template<class Executor>
struct is_bulk_oneway_executor : is_bulk_oneway_executor_impl::eval<Executor> {};

} // namespace execution
} // inline namespace executors_v1
} // namespace experimental
} // namespace std

#endif // STD_EXPERIMENTAL_BITS_IS_BULK_ONEWAY_EXECUTOR_H
