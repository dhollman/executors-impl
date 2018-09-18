
#pragma once

#include <experimental/execution>

template <class Executor, class Condition, class TrueSender, class FalseSender>
struct conditional_sender {
  Executor ex_;
  Condition c_;
  TrueSender ts_;
  FalseSender fs_;
  template <class R>
  void submit(R&& r) && {
    if(std::move(c_)()) {
      std::experimental::execution::submit(std::move(ts_), std::forward<R>(r));
    }
    else {
      std::experimental::execution::submit(std::move(fs_), std::forward<R>(r));
    }
  }
  static constexpr void query(std::experimental::execution::sender_t) noexcept { }
  constexpr auto executor() const { return ex_; }
};
template <class Executor, class Condition, class TrueSender, class FalseSender>
conditional_sender(Executor, Condition, TrueSender, FalseSender)
  -> conditional_sender<Executor, Condition, TrueSender, FalseSender>;

template <class Executor, class Condition, class TrueSenderFactory, class FalseSenderFactory>
struct lazy_conditional_sender {
  Executor ex_;
  Condition c_;
  TrueSenderFactory ts_;
  FalseSenderFactory fs_;
  template <class R>
  void submit(R&& r) && {
    if(std::move(c_)()) {
      std::experimental::execution::submit(std::move(ts_)(), std::forward<R>(r));
    }
    else {
      std::experimental::execution::submit(std::move(fs_)(), std::forward<R>(r));
    }
  }
  template <typename TrueSender>
  struct lazy_eval_sender_desc_t {
    using error_type = typename std::experimental::execution::sender_traits<TrueSender>::error_type;
    template <template <class...> class List>
    using value_types = typename std::experimental::execution::sender_traits<TrueSender>::template value_types<List>;
  };

  template <std::Same<lazy_conditional_sender> Sender>
  friend constexpr lazy_eval_sender_desc_t<decltype(std::declval<TrueSenderFactory>()())>
  query(Sender const&, std::experimental::execution::sender_description_t) noexcept { return {}; }

  static constexpr void query(std::experimental::execution::sender_t) noexcept { }

  constexpr auto executor() const { return ex_; }
};
template <class Executor, class Condition, class TrueSenderFactory, class FalseSenderFactory>
lazy_conditional_sender(Executor, Condition, TrueSenderFactory, FalseSenderFactory)
  -> lazy_conditional_sender<Executor, Condition, TrueSenderFactory, FalseSenderFactory>;