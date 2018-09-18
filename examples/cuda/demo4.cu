// $ nvcc -std=c++14 --expt-extended-lambda demo.cu
#include "cuda_stream_executor.hpp"
#include <iostream>
#include <utility>

namespace execution = std::experimental::execution;

template<class T, class Executor>
struct just
{
  using value_type = T;
  T value_;
  Executor ex_;


  template<class SingleReceiver>
  __host__ __device__
  void submit(SingleReceiver sr)
  {
    std::move(sr).set_value(value_);
  }

  auto executor() const { return ex_; }

  static constexpr void query(execution::sender_t) noexcept { }
  static constexpr execution::sender_desc<std::exception_ptr, T>
  query(execution::sender_description_t) noexcept { return { }; }
};
template <class T, class Executor>
just(T, Executor) -> just<T, Executor>;

struct printf_fn
{
  __host__ __device__
  void operator()(int val)
  {
    printf("Received %d in printf_receiver\n", val);
  }
};

void do_some_work()
{
  printf("Doing some work\n");
}

int main()
{

  cuda_stream_executor<> ex;
  
  auto task_a = ex.make_value_task(just{13, ex}, [] __host__ __device__ (int val)
  {
    printf("Received %d in task a\n", val);
    return val + 1;
  });

  auto shared_task_b = ex.make_value_task(std::move(task_a), [] __host__ __device__ (int val)
  {
    printf("Received %d in task b\n", val);
    return val + 1;
  }).share();
  
  // submit task b into a print function
  shared_task_b.submit(
    execution::receiver{execution::on_value{printf_fn()}}
  );

  // do some work on the current thread
  do_some_work();

  auto blocking_ex = execution::require(ex, execution::blocking.always);
  std::move(shared_task_b).via(blocking_ex).submit(execution::sink_receiver{});

  std::cout << "OK" << std::endl;

  return 0;
}

