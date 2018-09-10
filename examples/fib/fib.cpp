
#include <experimental/thread_pool>
#include <utility>
#include <optional>
#include <mutex>
#include <vector>
#include <iostream>
#include <tuple>

#include "join_handler.hpp"
#include "subject_factory.hpp"
#include "inline_executor.hpp"

namespace execution = std::experimental::execution;


template <class T, class Executor>
struct just_sender {
  T t_;
  Executor ex_;
  template <execution::Receiver R>
  void submit(R&& r) && {
    std::move(ex_).execute([t=std::move(t_), r_=std::forward<R>(r)]() mutable {
      std::move(r_).set_value(t);
    });
  }

  static constexpr execution::sender_desc<std::exception_ptr, T> query(execution::sender_t) { return { }; }

  auto executor() { return ex_; }
};
template <class T, class Executor>
just_sender(T, Executor) -> just_sender<T, Executor>;



template <class Executor, execution::ReceiverOf<std::exception_ptr, int> R>
void fib(int n, Executor ex, R r) {
  if(n < 2) {
    just_sender{n, ex}.submit(std::move(r));
  }
  else {
    auto subject_factory = execution::query(ex, execution::subject_factory<int>);
    auto subj_n_minus_1 = subject_factory.make_subject();
    auto subj_n_minus_2 = subject_factory.make_subject();
    fib(n-1, ex, subj_n_minus_1);
    fib(n-2, ex, subj_n_minus_2);
    auto joiner = execution::query(ex, execution::join_handler<>);
    ex.make_value_task(
      joiner.make_joined_sender(subj_n_minus_1, subj_n_minus_2),
      [](int v1, int v2) {
        return v1 + v2;
      }
    ).submit(std::move(r));
  }
}


int main() {

  auto cout_receiver = execution::receiver{
    execution::on_value{
      [](auto&& v) { std::cout << v << std::endl; }
    }
  };

  fib(10, inline_executor{}, cout_receiver);

  //auto pool = std::experimental::static_thread_pool{8};

  //fib(10, pool.executor(), cout_receiver);

  //pool.wait();

}
