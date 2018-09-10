#pragma once

#include <experimental/execution>

#include <utility>

struct inline_executor {
  template <class F, class R>
  struct __inline_receiver {
    F f_;
    R r_;
    template <class... Args>
    void set_value(Args&&... args) {
      std::move(r_).set_value(
        std::move(f_)(std::forward<Args>(args)...)
      );
    }
    template <class E>
    void set_error(E&& e) {
      std::move(r_).set_error(std::forward<E>(e));
    }
    void set_done() {
      std::move(r_).set_done();
    }

    static constexpr void query(std::experimental::execution::receiver_t) { }
  };

  template <class T, class E=std::exception_ptr>
  struct __subject {
    // TODO ????

  };

  template <class S, class F>
  struct __task_submit_fn {
    S s_;
    F f_;

    template <typename... Values>
    struct _value_types_helper {
      using type = std::invoke_result_t<F, Values...>;
    };

    template <std::Same<S> LazyS>
    using sender_desc_t = std::experimental::execution::sender_desc<
      std::exception_ptr,
      typename std::experimental::execution::sender_traits<LazyS>::template value_types<_value_types_helper>::type
    >;

    template <std::Same<S> LazyS>
    friend constexpr sender_desc_t<LazyS> query(
      __task_submit_fn<LazyS, F> const&,
      std::experimental::execution::sender_t
    ) noexcept { return { }; }

    
    template <class Receiver>
    void submit(Receiver&& r) {
      std::move(s_).submit(
        __inline_receiver<F, std::decay_t<Receiver>>{std::move(f_), std::forward<Receiver>(r)}
      );
    }
  };

  template <class NullaryFunction>
  void execute(NullaryFunction&& f) const {
    f();
  }

  template <std::experimental::execution::ReceiverOf<inline_executor> R>
  void submit(R&& r) const {
    std::forward<R>(r).set_value(*this);
  }

  template <std::experimental::execution::Sender S, class Function>
  auto make_value_task(S&& s, Function f) const {
    return std::experimental::execution::sender{
      __task_submit_fn<std::decay_t<S>, Function>{
        std::forward<S>(s), std::move(f)
      },
      []() { return inline_executor{}; }
    };
  }

  using sender_desc_t = std::experimental::execution::sender_desc<std::exception_ptr, inline_executor>;
  static constexpr sender_desc_t query(std::experimental::execution::sender_t) { return { }; }

};
