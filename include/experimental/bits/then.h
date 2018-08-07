#ifndef STD_EXPERIMENTAL_BITS_THEN_H
#define STD_EXPERIMENTAL_BITS_THEN_H

#include <experimental/execution>
#include <experimental/bits/adapter.h>

#include <optional>

namespace std {
namespace experimental {
namespace executors_v1 {
namespace execution {

namespace then_impl {


// TODO address stack overflow with a trampoline here
template <typename PrevContinuation, typename Continuation>
struct adapted_then_continuation {
  private:
    PrevContinuation pc_;
    Continuation cc_;

  public:

    adapted_then_continuation(adapted_then_continuation&&) noexcept = default;

    template <typename PrevContinuationDeduced, typename ContinuationDeduced>
    adapted_then_continuation(
      PrevContinuationDeduced&& pc,
      ContinuationDeduced&& cc
    ) : pc_(forward<PrevContinuationDeduced>(pc)),
        cc_(forward<ContinuationDeduced>(cc))
    { }

    template <class Value, class SubEx>
    auto value(Value&& val, SubEx subex) {
      if constexpr (!is_void_v<decay_t<decltype(declval<PrevContinuation&&>().value(declval<Value&&>(), declval<SubEx>()))>>) {
        optional<decay_t<decltype(std::move(pc_).value(forward<Value>(val), subex))>> nextval;
        try {
          nextval = std::move(pc_).value(forward<Value>(val), subex);
        }
        catch(...) {
          auto ex = std::move(cc_).error(std::current_exception(), std::move(subex));
          if(ex) std::rethrow_exception(ex);
        }
        return std::move(cc_).value(std::move(*nextval), std::move(subex));
      }
      else /* constexpr */ {
        // void return case:
        try {
          std::move(pc_).value(forward<Value>(val), subex);
        }
        catch(...) {
          auto ex = std::move(cc_).error(std::current_exception(), std::move(subex));
          if(ex) std::rethrow_exception(ex);
        }
        return std::move(cc_).value(std::move(subex));
      }
    }

    // void input case
    template <class SubEx>
    auto value(SubEx subex) {
      if constexpr (!is_void_v<decay_t<decltype(declval<PrevContinuation&&>().value(declval<SubEx>()))>>) {
        optional<decay_t<decltype(std::move(pc_).value(subex))>> nextval;
        try {
          nextval = std::move(pc_).value(subex);
        }
        catch(...) {
          auto ex = std::move(cc_).error(std::current_exception(), std::move(subex));;
          if(ex) std::rethrow_exception(ex);
        }
        return std::move(cc_).value(std::move(*nextval), std::move(subex));
      }
      else /* constexpr */ {
        // void return case:
        try {
          std::move(pc_).value(subex);
        }
        catch(...) {
          auto ex = std::move(cc_).error(std::current_exception(), std::move(subex));
          if(ex) std::rethrow_exception(ex);
        }
        return std::move(cc_).value(std::move(subex));
      }
    }

    template <class Error, class SubEx>
    auto error(Error&& err, SubEx subex) {
      // TODO: Is this the expected behavior, or should be be returning to the value chain if the error is handled?
      return std::move(cc_).error(
        std::move(pc_).error(forward<decltype(err)>(err), subex),
        std::move(subex)
      );
    }

};

template <typename PrevContinuationDeduced, typename ContinuationDeduced>
adapted_then_continuation(
  PrevContinuationDeduced&& pc,
  ContinuationDeduced&& cc
) -> adapted_then_continuation<decay_t<PrevContinuationDeduced>, decay_t<ContinuationDeduced>>;

template<class Executor, class Continuation> class adapter;

template<class Continuation>
struct continuation_wrapped_adapter_mfn {
  template<class Executor>
  using apply = adapter<Executor, Continuation>;
};

template<class Executor, class PrevContinuation>
class adapter : public impl::adapter<continuation_wrapped_adapter_mfn<PrevContinuation>::template apply, Executor>
{
    template <class _ignored> static auto inner_declval() -> decltype(std::declval<Executor>());
    template <class, class T> struct dependent_type { using type = T; };

    PrevContinuation prev_;

    template<class, class>
    friend class adapter;
    template<template<class> class, class>
    friend class impl::adapter;

    using base_t = impl::adapter<continuation_wrapped_adapter_mfn<PrevContinuation>::template apply, Executor>;


    template <typename PrevContinuationDeduced>
    adapter(std::in_place_t, Executor&& ex, PrevContinuationDeduced&& cont)
      : base_t(std::forward<Executor>(ex)), prev_(std::move(cont))
    { }

    template <typename OtherExecutor>
    adapter(Executor&& ex, adapter<OtherExecutor, PrevContinuation>&& old)
      : base_t(std::forward<Executor>(ex)), prev_(std::move(old.prev_))
    { }

  public:

    // TODO constrain to default constructible prev_
    explicit adapter(Executor&& ex) : base_t(std::move(ex)), prev_() { }

    template <class Continuation>
    auto execute(Continuation&& c) &&
      -> decltype(inner_declval<Continuation>().execute(std::move(c)))
    {
      return std::move(this->executor_).execute(
        adapted_then_continuation(
          std::move(prev_),
          forward<Continuation>(c)
        )
      );
    }

    template <class Continuation>
    auto then_execute(Continuation&& c) &&
    {
      // TODO check for continuation compatibility here rather than letting things fail further down
      // just wrap so that the continuation runs after the previous continuation.
      return adapter<Executor, adapted_then_continuation<PrevContinuation, decay_t<Continuation>>>(
        std::in_place,
        std::move(this->executor_),
        adapted_then_continuation(
          std::move(prev_),
          forward<Continuation>(c)
        )
      );
    }
};

} // end namespace then_impl

template<class T/*=void*/, class E/*=exception_ptr*/, class VSubEx/*=void*/, class ESubEx/*=void*/>
struct then_t
{
    static constexpr bool is_requirable = true;
    static constexpr bool is_preferable = false;

    using polymorphic_query_result_type = bool;

  private:

    template <typename Executor>
    using actual_vsubex_t = conditional_t<is_void_v<VSubEx>, Executor, VSubEx>;
    template <typename Executor>
    using actual_esubex_t = conditional_t<is_void_v<ESubEx>, Executor, ESubEx>;

    struct trivial_continuation {
      template <typename SubExec>
      inline constexpr decltype(auto) value(SubExec&&) noexcept { }
      template <typename Value, typename SubExec>
      inline constexpr decltype(auto) value(Value&& v, SubExec&&) noexcept { return std::forward<Value>(v); }
      template <typename Error, typename SubExec>
      inline constexpr decltype(auto) error(Error&& e, SubExec&&) noexcept { return std::forward<Error>(e); }
    };

  public:
    template<class Executor>
    static constexpr bool static_query_v
      = is_then_executor_v<Executor, T, E, actual_vsubex_t<Executor>, actual_esubex_t<Executor>>;

    static constexpr bool value() { return true; }

  public:
    // Default require for bulk adapts single executors.
    template <typename Executor, typename =
      std::enable_if_t<
        is_executor_v<Executor, T, E, actual_vsubex_t<Executor>, actual_esubex_t<Executor>>
          && !is_then_executor_v<Executor, T, E, actual_vsubex_t<Executor>, actual_esubex_t<Executor>>
      >
    >
    friend then_impl::adapter<Executor, trivial_continuation> require(Executor&& ex, then_t)
    {
      // TODO impose restrictions w.r.t. blocking...
      return then_impl::adapter<Executor, trivial_continuation>(std::move(ex));
    }
};

template<class T=void, class E=exception_ptr, class VSubEx=void, class ESubEx=void>
constexpr then_t<T, E, VSubEx, ESubEx> then;

} // end namespace execution
} // end namespace executors_v1
} // end namespace experimental
} // end namespace std

#endif //STD_EXPERIMENTAL_BITS_THEN_H
