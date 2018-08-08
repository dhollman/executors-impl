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


// Used for "kicking off" chain
struct _trivial_continuation {
  template <typename SubExec>
  inline constexpr auto value(SubExec&&) noexcept { }
  template <typename Value, typename SubExec>
  inline constexpr auto value(Value&& v, SubExec&&) noexcept { return std::forward<Value>(v); }
  template <typename Error, typename SubExec>
  inline constexpr auto error(Error&& e, SubExec&&) noexcept { return std::forward<Error>(e); }
};



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
        if constexpr(!is_same_v<PrevContinuation, _trivial_continuation>) {
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
        else /* constexpr (PrevContinuation is _trivial_continuation) */ {
          return std::move(cc_).value(std::forward<Value>(val), std::move(subex));
        }
      }
      else /* constexpr */ {
        // void return case:
        if constexpr(!is_same_v<PrevContinuation, _trivial_continuation>) {
          // this if constexpr is really unnecessary, since the compiler should
          // figure out nothing is happening here in the trivial case
          try {
            std::move(pc_).value(forward<Value>(val), subex);
          }
          catch(...) {
            auto ex = std::move(cc_).error(std::current_exception(), std::move(subex));
            if(ex) std::rethrow_exception(ex);
          }
        }
        return std::move(cc_).value(std::move(subex));
      }
    }

    // void input case
    template <class SubEx>
    auto value(SubEx subex) {
      if constexpr (!is_void_v<decay_t<decltype(declval<PrevContinuation&&>().value(declval<SubEx>()))>>) {
        // PrevContinuation can't be _trivial_continuation since we went from void to non-void
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
        if constexpr(!is_same_v<PrevContinuation, _trivial_continuation>) {
          // this if constexpr is really unnecessary, since the compiler should
          // figure out nothing is happening here in the trivial case
          try {
            std::move(pc_).value(subex);
          }
          catch(...) {
            auto ex = std::move(cc_).error(std::current_exception(), std::move(subex));
            if(ex) std::rethrow_exception(ex);
          }
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

template<class Executor, class Continuation, class PrevValue, class PrevError> class adapter;

template<class Continuation, class PrevValue, class PrevError>
struct continuation_wrapped_adapter_mfn {
  template<class Executor>
  using apply = adapter<Executor, Continuation, PrevValue, PrevError>;
};

// For use with executor_value and executor_error in unevaluated contexts
struct _trivial_inline_unevaluated_executor {
  using value_type = void;
  template <class Continuation>
  void execute(Continuation&&) {
    /* give an implementation so it links when used with the polymorphic wrapper */
    std::abort(); // unreachable
  }
};

// TODO clean up the assumption here that PrevContinuation.value() and .error() can be called with trivial_inline_executor (should probably be executor_value_subexecutor_t<Executor, Continuation> or something)
// Putting executor here also for we ever decide we need to query the subexecutor
template <class Executor, class Continuation, class Value>
struct _continuation_value_return {
  using type = decltype(declval<Continuation>().value(declval<Value>(), then_impl::_trivial_inline_unevaluated_executor{}));
};
template <class Executor, class Continuation>
struct _continuation_value_return<Executor, Continuation, void> {
  using type = decltype(declval<Continuation>().value(then_impl::_trivial_inline_unevaluated_executor{}));
};
template <class Executor, class Continuation, class Error>
struct _continuation_error_return {
  using type = decltype(declval<Continuation>().error(declval<Error>(), then_impl::_trivial_inline_unevaluated_executor{}));
};

template<class Executor, class PrevContinuation, class PrevValue, class PrevError>
class adapter : public impl::adapter<continuation_wrapped_adapter_mfn<PrevContinuation, PrevValue, PrevError>::template apply, Executor>
{
    template <class _ignored> static auto inner_declval() -> decltype(std::declval<Executor>());
    template <class, class T> struct dependent_type { using type = T; };

    PrevContinuation prev_;

    template<class, class, class, class>
    friend class adapter;
    template<template<class> class, class>
    friend class impl::adapter;

    using base_t = impl::adapter<continuation_wrapped_adapter_mfn<PrevContinuation, PrevValue, PrevError>::template apply, Executor>;


    template <typename PrevContinuationDeduced>
    adapter(std::in_place_t, Executor&& ex, PrevContinuationDeduced&& cont)
      : base_t(std::forward<Executor>(ex)), prev_(std::move(cont))
    { }

    template <typename OtherExecutor>
    adapter(Executor&& ex, adapter<OtherExecutor, PrevContinuation, PrevValue, PrevError>&& old)
      : base_t(std::forward<Executor>(ex)), prev_(std::move(old.prev_))
    { }

  public:

    // Conditional move-from-executor constructor
    template <class _ignored=void,
      class=enable_if_t<
        is_nothrow_default_constructible_v<PrevContinuation>
          && is_void_v<_ignored>
      >
    >
    explicit adapter(Executor&& ex) noexcept : base_t(std::move(ex)), prev_() { }

    // Conditional copy-from-executor constructor
    template <class _ignored=void,
      class=enable_if_t<
        is_nothrow_default_constructible_v<PrevContinuation>
          && is_copy_constructible_v<Executor>
          && is_void_v<_ignored>
      >
    >
    explicit adapter(Executor const& ex) : base_t(ex), prev_() { }

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
      using next_adapter_t = adapter<
        Executor,
        adapted_then_continuation<PrevContinuation, decay_t<Continuation>>,
        typename _continuation_value_return<Executor, PrevContinuation, PrevValue>::type,
        typename _continuation_error_return<Executor, PrevContinuation, PrevError>::type
      >;
      return next_adapter_t(
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

// partial specialization of executor_value for the then_impl adapter
template <class Executor, class PrevContinuation, class PrevValue, class PrevError>
struct executor_value<then_impl::adapter<Executor, PrevContinuation, PrevValue, PrevError>> {
  using type = typename then_impl::_continuation_value_return<Executor, PrevContinuation, PrevValue>::type;
};

// partial specialization of executor_error for the then_impl adapter
template <class Executor, class PrevContinuation, class PrevValue, class PrevError>
struct executor_error<then_impl::adapter<Executor, PrevContinuation, PrevValue, PrevError>> {
  // TODO update this when we figure out what the expected error behavior is for a then_executor
  using type = typename then_impl::_continuation_error_return<Executor, PrevContinuation, PrevError>::type;
};

template<class VSubEx/*=void*/, class ESubEx/*=void*/>
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


  public:
    template<class Executor>
    static constexpr bool static_query_v
      = is_then_executor_v<Executor, executor_value_t<Executor>, executor_error_t<Executor>, actual_vsubex_t<Executor>, actual_esubex_t<Executor>>;

    static constexpr bool value() { return true; }

  public:
    // Default require for bulk adapts single executors.
    template <typename Executor, typename =
      std::enable_if_t<
        is_executor_v<Executor, executor_value_t<decay_t<Executor>>, executor_error_t<decay_t<Executor>>, actual_vsubex_t<Executor>, actual_esubex_t<Executor>>
          && !is_then_executor_v<Executor, executor_value_t<decay_t<Executor>>, executor_error_t<decay_t<Executor>>, actual_vsubex_t<Executor>, actual_esubex_t<Executor>>
      >
    >
    friend auto require(Executor&& ex, then_t)
    {
      return then_impl::adapter<
        decay_t<Executor>,
        then_impl::_trivial_continuation,
        executor_value_t<decay_t<Executor>>,
        executor_error_t<decay_t<Executor>>
      >(std::forward<Executor>(ex));
    }
};

template<class VSubEx=void, class ESubEx=void>
constexpr then_t<VSubEx, ESubEx> then;

} // end namespace execution
} // end namespace executors_v1
} // end namespace experimental
} // end namespace std

#endif //STD_EXPERIMENTAL_BITS_THEN_H
