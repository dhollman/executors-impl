

#include <iostream>
#include <experimental/execution>

struct inline_executor_factory;

struct inline_executor {
  using value_type = void;
  template <typename Continuation>
  void execute(Continuation&& c) {
    std::forward<Continuation>(c).value();
  }
  constexpr bool operator==(inline_executor const& other) const noexcept { return true; }
  constexpr bool operator!=(inline_executor const& other) const noexcept { return false; }
  template <typename U, typename E>
  static constexpr auto query(std::experimental::execution::executor_factory_t<U, E>) noexcept;
};

template <typename Value>
struct inline_value_executor {
  using value_type = Value;
  Value v;
  template <typename Continuation>
  void execute(Continuation&& c) && {
    std::forward<Continuation>(c).value(std::move(v));
  }
  constexpr bool operator==(inline_value_executor const& other) const noexcept { return v == other.v; }
  constexpr bool operator!=(inline_value_executor const& other) const noexcept { return v != other.v; }
  template <typename U, typename E>
  static constexpr auto query(std::experimental::execution::executor_factory_t<U, E>) noexcept;
};
template <typename Value>
inline_value_executor(Value) -> inline_value_executor<Value>;

template <typename Error>
struct inline_error_executor {
  using value_type = void;
  using error_type = Error;
  Error e;
  template <typename Continuation>
  void execute(Continuation&& c) && {
    std::forward<Continuation>(c).error(std::move(e));
  }
  constexpr bool operator==(inline_error_executor const& other) const noexcept { return e == other.e; }
  constexpr bool operator!=(inline_error_executor const& other) const noexcept { return e != other.e; }
  template <typename U, typename E>
  static constexpr auto query(std::experimental::execution::executor_factory_t<U, E>) noexcept;
};
template <typename Error>
inline_error_executor(Error) -> inline_error_executor<Error>;

struct inline_executor_factory {
  auto make_executor() { return inline_executor{};}
  template <typename T>
  auto make_executor(T&& t) { return inline_value_executor{t};}
  template <typename E>
  auto make_error_executor(E&& e) { return inline_error_executor{std::forward<E>(e)};}
};

template <typename U, typename E>
constexpr auto
inline_executor::query(std::experimental::execution::executor_factory_t<U, E>) noexcept {
  return inline_executor_factory{};
}
template <typename Value>
template <typename U, typename E>
constexpr auto
inline_value_executor<Value>::query(std::experimental::execution::executor_factory_t<U, E>) noexcept {
  return inline_executor_factory{};
}
template <typename Error>
template <typename U, typename E>
constexpr auto
inline_error_executor<Error>::query(std::experimental::execution::executor_factory_t<U, E>) noexcept {
  return inline_executor_factory{};
}

//==============================================================================
//==============================================================================
//==============================================================================

int main() {
  namespace execution = std::experimental::execution;
  using std::experimental::execution::then;
  using std::experimental::execution::blocking_adaptation;

  // add a property at the beginning of the chain
  auto then_ex = execution::require(
    inline_value_executor{42}, blocking_adaptation.allowed, then
  );
  auto next_ex = execution::require(
    std::move(then_ex).then_execute(execution::on_value([](int v){ std::cout << "hello, " << v << std::endl; return v*v; })),
    then
  );
  auto tail_ex = std::move(next_ex).then_execute(execution::on_value([](int v){ std::cout << "world, " << v << std::endl; return 73; }));

  // Make sure it's still there
  static_assert(execution::query(tail_ex, blocking_adaptation) == blocking_adaptation.allowed);

  auto blk_ex = execution::require(std::move(tail_ex), execution::blocking.always);

  int i = 5;
  auto tail_ex2 = std::move(blk_ex).then_execute(execution::on_value([&i](int newi) { i = newi; return 42; }));
  assert(i == 73);

  // remove a property
  auto noblock_tail_ex = execution::require(std::move(tail_ex2), blocking_adaptation.disallowed);

  // Make sure it got removed
  static_assert(execution::query(noblock_tail_ex, blocking_adaptation) == blocking_adaptation.disallowed);

  // And that it still works:
  std::move(noblock_tail_ex)
    .execute(execution::on_value([](int val){ std::cout << "goodbye, " << val << std::endl; }));


  // (potentially?) less generic option: require `then_execute` to always return a `ThenExecutor` of the appropriate type
  std::experimental::execution::require(inline_value_executor{42}, then)
    .then_execute(execution::on_value([](auto&& val){ std::cout << "hello again, " << val << std::endl; }))
    .then_execute(execution::on_void([](){ std::cout << "world, " << std::endl; return 73; }))
    .execute(execution::on_value([](int val){ std::cout << "goodbye, " << val << std::endl; }));

}