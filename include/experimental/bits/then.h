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
  inline constexpr auto value() noexcept { }
  template <typename Value>
  inline constexpr auto value(Value&& v) noexcept { return std::forward<Value>(v); }
  template <typename Error>
  inline constexpr auto error(Error&& e) noexcept { return std::forward<Error>(e); }
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

    template <class Value>
    auto value(Value&& val) {
      using prev_value_t = decay_t<decltype(declval<PrevContinuation&&>().value(declval<Value&&>()))>;
      if constexpr (!is_void_v<prev_value_t>) {
        if constexpr(!is_same_v<PrevContinuation, _trivial_continuation>) {
          optional<prev_value_t> nextval;
          try {
            nextval = std::move(pc_).value(forward<Value>(val));
          }
          catch(...) {
            auto ex = std::move(cc_).error(std::current_exception());
            if(ex) std::rethrow_exception(ex);
          }
          return std::move(cc_).value(std::move(*nextval));
        }
        else /* constexpr (PrevContinuation is _trivial_continuation) */ {
          return std::move(cc_).value(std::forward<Value>(val));
        }
      }
      else /* constexpr */ {
        // void return case:
        if constexpr(!is_same_v<PrevContinuation, _trivial_continuation>) {
          // this if constexpr is really unnecessary, since the compiler should
          // figure out nothing is happening here in the trivial case
          try {
            std::move(pc_).value(forward<Value>(val));
          }
          catch(...) {
            auto ex = std::move(cc_).error(std::current_exception());
            if(ex) std::rethrow_exception(ex);
          }
        }
        return std::move(cc_).value();
      }
    }

    // void input case
    auto value() {
      using prev_value_t = decay_t<decltype(declval<PrevContinuation&&>().value())>;
      if constexpr (!is_void_v<prev_value_t>) {
        // PrevContinuation can't be _trivial_continuation since we went from void to non-void
        optional<prev_value_t> nextval;
        try {
          nextval = std::move(pc_).value();
        }
        catch(...) {
          auto ex = std::move(cc_).error(std::current_exception());
          if(ex) std::rethrow_exception(ex);
        }
        return std::move(cc_).value(std::move(*nextval));
      }
      else /* constexpr */ {
        // void return case:
        if constexpr(!is_same_v<PrevContinuation, _trivial_continuation>) {
          // this if constexpr is really unnecessary, since the compiler should
          // figure out nothing is happening here in the trivial case
          try {
            std::move(pc_).value();
          }
          catch(...) {
            auto ex = std::move(cc_).error(std::current_exception());
            if(ex) std::rethrow_exception(ex);
          }
        }
        return std::move(cc_).value();
      }
    }

    template <class Error>
    auto error(Error&& err) {
      // TODO: Is this the expected behavior, or should be be returning to the value chain if the error is handled?
      return std::move(cc_).error(
        std::move(pc_).error(forward<Error>(err))
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

template <class Executor, class Continuation, class Value>
struct _continuation_value_return {
  using type = decltype(declval<Continuation>().value(declval<Value>()));
};
template <class Executor, class Continuation>
struct _continuation_value_return<Executor, Continuation, void> {
  using type = decltype(declval<Continuation>().value());
};
template <class Executor, class Continuation, class Error>
struct _continuation_error_return {
  using type = decltype(declval<Continuation>().error(declval<Error>()));
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

struct then_t
{
  static constexpr bool is_requirable = true;
  static constexpr bool is_preferable = false;

  using polymorphic_query_result_type = bool;

  template<class Executor>
  static constexpr bool static_query_v
    = is_then_executor_v<Executor, executor_value_t<Executor>, executor_error_t<Executor>>;

  static constexpr bool value() { return true; }

  // Default require for bulk adapts single executors.
  template <typename Executor, typename =
    std::enable_if_t<
      is_executor_v<Executor, executor_value_t<decay_t<Executor>>, executor_error_t<decay_t<Executor>>>
        && !is_then_executor_v<Executor, executor_value_t<decay_t<Executor>>, executor_error_t<decay_t<Executor>>>
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

constexpr then_t then;

} // end namespace execution
} // end namespace executors_v1
} // end namespace experimental
} // end namespace std

#endif //STD_EXPERIMENTAL_BITS_THEN_H
