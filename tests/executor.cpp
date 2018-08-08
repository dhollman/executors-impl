#include <experimental/execution>
//#include <experimental/thread_pool>

namespace execution = std::experimental::execution;



using executor = execution::executor<void, std::exception_ptr,
    execution::mapping_t::thread_t,
    execution::blocking_t::possibly_t,
    execution::blocking_t::always_t
  >;
using int_executor = execution::executor<int, std::exception_ptr,
  execution::mapping_t::thread_t,
  execution::blocking_t::possibly_t,
  execution::blocking_t::always_t
>;

struct inline_executor {
  using value_type = void;
  template <class Continuation>
  void execute(Continuation&& c) {
    std::forward<Continuation>(c).value(*this);
  }
  static constexpr auto query(execution::blocking_t) noexcept { return execution::blocking.always; }
  auto require(execution::blocking_t::possibly_t) && noexcept { return std::move(*this); }
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
  static constexpr auto query(execution::blocking_t) noexcept { return execution::blocking.always; }
  auto require(execution::blocking_t::possibly_t) && noexcept { return std::move(*this); }
  constexpr bool operator==(inline_value_executor const&) const noexcept { return true; }
  constexpr bool operator!=(inline_value_executor const&) const noexcept { return false; }
};
template <class Value>
inline_value_executor(Value) -> inline_value_executor<Value>;

void executor_compile_test()
{
  static_assert(noexcept(executor()), "default constructor must not throw");
  static_assert(noexcept(executor(nullptr)), "nullptr constructor must not throw");

  executor ex1;
  executor ex2(nullptr);

  static_assert(noexcept(executor(std::move(ex1))), "move constructor must not throw");

  executor ex4(std::move(ex1));

  executor ex5(inline_executor{});

  execution::executor<void, std::exception_ptr> ex6(std::move(ex5));

  int_executor ex7(inline_value_executor<int>{42});

  //static_assert(noexcept(ex2 = cex1), "copy assignment must not throw");
  static_assert(noexcept(ex1 = std::move(ex2)), "move assignment must not throw");
  static_assert(noexcept(ex1 = nullptr), "nullptr assignment must not throw");

  ex1 = std::move(ex2);

  static_assert(noexcept(ex1.swap(ex2)), "swap must not throw");

  ex1.swap(ex2);

  ex1.assign(inline_executor{});

  ex1 = execution::require(std::move(ex1), execution::mapping.thread);
  ex1 = execution::require(std::move(ex1), execution::blocking.possibly);
  ex1 = execution::require(std::move(ex1), execution::blocking.always);

  ex1 = execution::prefer(std::move(ex1), execution::mapping.thread);
  ex1 = execution::prefer(std::move(ex1), execution::blocking.never);
  ex1 = execution::prefer(std::move(ex1), execution::blocking.possibly);
  ex1 = execution::prefer(std::move(ex1), execution::blocking.always);
  ex1 = execution::prefer(std::move(ex1), execution::relationship.fork);
  ex1 = execution::prefer(std::move(ex1), execution::relationship.continuation);
  ex1 = execution::prefer(std::move(ex1), execution::outstanding_work.untracked);
  ex1 = execution::prefer(std::move(ex1), execution::outstanding_work.tracked);
  ex1 = execution::prefer(std::move(ex1), execution::bulk_guarantee.sequenced);
  ex1 = execution::prefer(std::move(ex1), execution::bulk_guarantee.parallel);
  ex1 = execution::prefer(std::move(ex1), execution::bulk_guarantee.unsequenced);
  ex1 = execution::prefer(std::move(ex1), execution::mapping.new_thread);

  std::move(ex1).execute(execution::on_void([]{}));

  bool b1 = static_cast<bool>(ex1);
  (void)b1;

  const std::type_info& target_type = ex5.target_type();
  (void)target_type;

  const inline_executor* cex6 = ex1.target<inline_executor>();
  (void)cex6;
  const inline_value_executor<int>* cex7 = ex7.target<inline_value_executor<int>>();
  (void)cex7;

  bool b2 = (ex6 == ex5);
  (void)b2;

  bool b3 = (ex6 != ex5);
  (void)b3;

  bool b4 = (ex1 == nullptr);
  (void)b4;

  bool b5 = (ex1 != nullptr);
  (void)b5;

  bool b6 = (nullptr == ex1);
  (void)b6;

  bool b7 = (nullptr != ex1);
  (void)b7;

  swap(ex1, ex2);
}

int main()
{
}
