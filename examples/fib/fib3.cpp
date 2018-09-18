


#include "unwrap_handler.hpp"
#include "join_handler.hpp"
#include "inline_executor.hpp"
#include "eager_inline_executor.hpp"
#include "conditional_sender.hpp"

#include <experimental/execution>
#include <iostream>


namespace execution = std::experimental::execution;

template <class Executor>
struct Fib {
  int n;
  Executor ex;

  auto make_joined_task() const {
    auto joiner = execution::query(ex, execution::join_handler<>);
    return ex.make_value_task(
      joiner.make_joined_sender(Fib{n-1, ex}, Fib{n-2, ex}),
      [](int v1, int v2) { return v1 + v2; }
    );
  }

  template <class Receiver>
  void submit(Receiver&& r) const {
    if(n < 2) {
      std::move(r).set_value(n);
    }
    else {
      make_joined_task().submit(std::forward<Receiver>(r));
      //make_joined_task().submit(
      //  execution::any_receiver<std::exception_ptr, int>{
      //    std::forward<Receiver>(r)
      //  }
      //);
    }
  }

  auto executor() const { return ex; }
  static constexpr void query(execution::sender_t) noexcept { }

};
template <class Executor>
Fib(int, Executor) -> Fib<Executor>;


auto fib_normal(int i) {
  if(i < 2) { return i; }
  else {
    return fib_normal(i-1) + fib_normal(i-2);
  }
}


int main() {
  std::cout << "fib_normal(10) = " << fib_normal(10) << std::endl;

  Fib{10, inline_executor{}}.submit(
    execution::receiver{execution::on_value{
      [](auto&& value) {
        std::cout << "fib_executors(10) = " << value << std::endl;
      }
    }}
  );

}