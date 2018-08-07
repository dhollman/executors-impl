

#include <iostream>
#include <experimental/execution>

struct inline_executor {
  template <typename Continuation>
  void execute(Continuation&& c) {
    std::forward<Continuation>(c).value(*this);
  }
  constexpr bool operator==(inline_executor const& other) const noexcept { return true; }
  constexpr bool operator!=(inline_executor const& other) const noexcept { return false; }
};

template <typename Value>
struct inline_value_executor {
  Value v;
  template <typename Continuation>
  void execute(Continuation&& c) && {
    std::forward<Continuation>(c).value(std::move(v), inline_executor{});
  }
  constexpr bool operator==(inline_value_executor const& other) const noexcept { return true; }
  constexpr bool operator!=(inline_value_executor const& other) const noexcept { return false; }

};
template <typename Value>
inline_value_executor(Value) -> inline_value_executor<Value>;


int main() {
  namespace execution = std::experimental::execution;
  using std::experimental::execution::then;
  using std::experimental::execution::blocking_adaptation;

  // add a property at the beginning of the chain
  auto then_ex = execution::require(
    inline_value_executor{42}, blocking_adaptation.allowed, then<int>
  );
  auto next_ex = execution::require(
    std::move(then_ex).then_execute(execution::on_value([](int v){ std::cout << "hello, " << v << std::endl; return v*v; })),
    then<int>
  );
  auto tail_ex = std::move(next_ex).then_execute(execution::on_value([](int v){ std::cout << "world, " << v << std::endl; return 73; }));

  // Make sure it's still there
  static_assert(execution::query(tail_ex, blocking_adaptation) == blocking_adaptation.allowed);

  // remove a property
  auto noblock_tail_ex = execution::require(std::move(tail_ex), blocking_adaptation.disallowed);

  // Make sure it got removed
  static_assert(execution::query(noblock_tail_ex, blocking_adaptation) == blocking_adaptation.disallowed);

  // And that it still works:
  std::move(noblock_tail_ex)
    .execute(execution::on_value_with_subexecutor([](int val, auto&&){ std::cout << "goodbye, " << val << std::endl; }));


  // (potentially?) less generic option: require `then_execute` to always return a `ThenExecutor` of the appropriate type
  std::experimental::execution::require(inline_value_executor{42}, then<int>)
    .then_execute(execution::on_value([](auto&& val){ std::cout << "hello again, " << val << std::endl; }))
    .then_execute(execution::on_void([](){ std::cout << "world, " << std::endl; return 73; }))
    .execute(execution::on_value([](int val){ std::cout << "goodbye, " << val << std::endl; }));



}