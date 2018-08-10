
#include <experimental/execution>

#include <utility>

namespace execution = std::experimental::execution;


template <typename Executor>
void adapters_compile_test(Executor ex) {

  auto bl_e = execution::require(std::move(ex), execution::blocking_adaptation.allowed);

  static_assert(std::is_same_v<
    execution::executor_value_t<Executor>,
    execution::executor_value_t<decltype(bl_e)>
  >);
  static_assert(std::is_same_v<
    execution::executor_error_t<Executor>,
    execution::executor_error_t<decltype(bl_e)>
  >);


  auto then_e = execution::require(std::move(bl_e), execution::then);

  static_assert(std::is_same_v<
    execution::executor_value_t<Executor>,
    execution::executor_value_t<decltype(then_e)>
  >);
  static_assert(std::is_same_v<
    execution::executor_error_t<Executor>,
    execution::executor_error_t<decltype(then_e)>
  >);

  // void version will call with std::monostate, so this works fine.
  auto next_e = std::move(then_e).then_execute(execution::on_value([](auto&&){ return 42; }));

  static_assert(std::is_same_v<
    execution::executor_value_t<decltype(next_e)>, int
  >);

}

//==============================================================================

template <typename Executor>
void generate_tests(Executor ex) {
  adapters_compile_test(std::move(ex));
}

//==============================================================================

struct inline_executor {
  using value_type = void;
  template <class Continuation>
  void execute(Continuation&& c) {
    std::forward<Continuation>(c).value(*this);
  }
  constexpr bool operator==(inline_executor const&) const noexcept { return true; }
  constexpr bool operator!=(inline_executor const&) const noexcept { return false; }
};

template <class Value>
struct inline_value_executor {
  using value_type = Value;
  Value v;
  template <class Continuation>
  void execute(Continuation&& c) {
    std::forward<Continuation>(c).value(v, *this);
  }
  constexpr bool operator==(inline_value_executor const&) const noexcept { return true; }
  constexpr bool operator!=(inline_value_executor const&) const noexcept { return false; }
};
template <class Value>
inline_value_executor(Value) -> inline_value_executor<Value>;

//==============================================================================

int main() {
  generate_tests(inline_executor{});
  generate_tests(inline_value_executor{3.14});

}
