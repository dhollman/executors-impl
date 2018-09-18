
#include "unwrap_handler.hpp"
#include "join_handler.hpp"
#include "inline_executor.hpp"
#include "eager_inline_executor.hpp"
#include "conditional_sender.hpp"

#include <experimental/execution>
#include <iostream>


namespace execution = std::experimental::execution;

namespace execution = std::experimental::execution;

struct Fib {
  int n;
  template <class Executor>
  auto operator()(Executor ex) {
    return lazy_conditional_sender{
      ex,
      // if:
      [n=n]{ return n < 2; },
      // then:
      [n=n,ex]{
        return ex.make_value_task(ex, [n=n](auto&&) { return n; });
      },
      // else:
      [n=n,ex]{
        auto t1 = ex.make_value_task(ex, Fib{n-1});
        auto t2 = ex.make_value_task(ex, Fib{n-2});
        auto unwrapper = execution::query(ex, execution::unwrap_handler);
        auto joiner = execution::query(ex, execution::join_handler<>);
        return ex.make_value_task(
          joiner.make_joined_sender(
            unwrapper.make_unwrapped_sender(std::move(t1)),
            unwrapper.make_unwrapped_sender(std::move(t2))
          ),
          [](int v1, int v2) {
            return v1 + v2;
          }
        );
      }
    };
  }
};

int main() {

  Fib{10}(eager_inline_executor{}).submit(
    execution::receiver{execution::on_value{
      [](int value) {
        std::cout << "fib(10) = " << value << std::endl;
      }
    }}
  );

}