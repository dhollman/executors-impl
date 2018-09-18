

#pragma once

#include "cuda_stream_fwd.hpp"
#include "event_lifetime.hpp"
#include "stream_task.hpp"

namespace std {
namespace experimental {
inline namespace executors_v1 {
namespace execution {

struct sink_receiver {
  static constexpr void query(receiver_t) noexcept { }
  template <typename... Args>
  constexpr inline void set_value(Args&&...) noexcept { }
  template <typename E>
  constexpr inline void set_error(E&&) noexcept { }
  constexpr inline void set_done() noexcept { }
};

}}}} // end namespace std::experimental::executors_v1::execution

namespace detail {

template <class Receiver, class Executor>
__global__
void _cuda_stream_executor_kernel(Receiver r, Executor ex) {
  std::move(r).set_value(std::move(ex));
}

} // end namespace detail

template <typename BlockingProperty>
class cuda_stream_executor {
  private:

    cudaStream_t stream_ = nullptr;

    __host__ __device__
    static cudaStream_t make_cuda_stream() {
      cudaStream_t result{};
      CHECK_CUDA_ERROR(cudaStreamCreateWithFlags(&result, cudaStreamNonBlocking));
      ENSURES_NOT_NULL(result);
      return result;
    }

    __host__ __device__
    static void destroy_cuda_stream(cudaStream_t stream) {
      EXPECTS_NOT_NULL(stream);
      CHECK_CUDA_ERROR(cudaStreamDestroy(stream));
    }

    __host__ __device__
    static auto make_cuda_event() {
      cudaEvent_t result{};
      CHECK_CUDA_ERROR(cudaEventCreateWithFlags(&result, cudaEventDisableTiming));
      ENSURES_NOT_NULL(result);
      return result;
    }

    __host__ __device__
    void record_stream_event(cudaEvent_t event) {
      EXPECTS_NOT_NULL(event);
      EXPECTS_NOT_NULL(stream_);
      CHECK_CUDA_ERROR(cudaEventRecord(event, stream_));
    }

    __host__ __device__
    void wait_stream_event(cudaEvent_t event) {
      EXPECTS_NOT_NULL(event);
      EXPECTS_NOT_NULL(stream_);
      CHECK_CUDA_ERROR(cudaStreamWaitEvent(stream_, event, 0));
    }

    /// Make an analog of this, but wrapping a different stream
    __host__ __device__
    auto make_analogous_executor_with_new_stream() const {
      EXPECTS_NOT_NULL(stream_);
      
      //unsigned int flags = 0;
      //CHECK_CUDA_ERROR(cudaStreamGetFlags(stream_, &flags));

      //int priority = 0;
      //CHECK_CUDA_ERROR(cudaStreamGetPriority(stream_, &priority));

      //cudaStream_t rstream{};
      //CHECK_CUDA_ERROR(cudaStreamCreateWithPriority(&rstream, flags, priority));

      return cuda_stream_executor(make_cuda_stream());
    }

    template <class Receiver>
    void _post_submit_actions(
      Receiver const& r,
      __REQUIRES(
        std::is_copy_constructible<Receiver>::value
        // the rest of this would be taken care of by concept subsumption
        && not std::is_same<BlockingProperty, std::experimental::execution::blocking_t::always_t>::value
      )
    ) {
      /* nothing to do for non-blocking case */
    }

    template <class Receiver>
    void _post_submit_actions(
      Receiver const& r,
      __REQUIRES(
        std::is_copy_constructible<Receiver>::value
        && std::is_same<BlockingProperty, std::experimental::execution::blocking_t::always_t>::value
      )
    ) {
      // blocking case with no special knowledge of the receiver; nothing else to do but block on the stream
      CHECK_CUDA_ERROR(cudaStreamSynchronize(stream_));
    }

    template <class Sender, class WrappedReceiver>
    void _post_submit_actions(
      detail::_cuda_stream_task_receiver<Sender, WrappedReceiver>&& r,
      __REQUIRES(
        // Sender should never be anything but one of our stream senders, but just in case...
        detail::is_stream_sender<Sender>::value
        // the rest of this would be taken care of by concept subsumption
        && not std::is_same<BlockingProperty, std::experimental::execution::blocking_t::always_t>::value
      )
    ) {
      // In the case where we know this is one of our stream tasks, we can just wait on the event
      CHECK_CUDA_ERROR(cudaEventSynchronize(r.sender_ptr->event()));
    }


    template <class, class>
    friend class detail::unique_stream_task;
    template <class, class>
    friend class detail::shared_stream_task;
    template <class>
    friend class cuda_stream_executor;

    explicit cuda_stream_executor(cudaStream_t stream)
      : stream_(stream)
    { }

    template <class OtherBlocking>
    explicit cuda_stream_executor(cuda_stream_executor<OtherBlocking> const& other)
      : stream_(other.stream_)
    { }

    template <class OtherBlocking>
    explicit cuda_stream_executor(cuda_stream_executor<OtherBlocking>&& other)
      : stream_(std::move(other.stream_))
    {
      other.stream_ = nullptr;
    }

  public:

    cuda_stream_executor(cuda_stream_executor const&) = default;
    cuda_stream_executor(cuda_stream_executor&&) = default;
    cuda_stream_executor& operator=(cuda_stream_executor const&) = default;
    cuda_stream_executor& operator=(cuda_stream_executor&&) = default;
    ~cuda_stream_executor() = default;

    cuda_stream_executor()
      : cuda_stream_executor(make_cuda_stream())
    { /* forwarding ctor, must be empty */ }
    
    // Generic sender, copyable function, copyable sender value
    template <typename Sender, typename Function>
    detail::unique_stream_task<
      cuda_stream_executor,
      detail::sender_result_t<Sender, Function const&>
    >
    make_value_task(Sender&& s, Function const& f,
      __REQUIRES(
        not detail::is_stream_sender<std::decay_t<Sender>>::value
        && std::is_copy_constructible<Function>::value
        && not std::is_void<sender_value_t<std::decay_t<Sender>>>::value
      )
    )
    {
      EXPECTS_NOT_NULL(stream_);

      using value_type = sender_value_t<std::decay_t<Sender>>;
      using result_type = decltype(f(std::declval<value_type>()));

      auto storage = detail::unique_value_storage<result_type>::make_storage();

      {
        // because cuda doesn't support init-capture yet...
        auto* result_ptr = storage.get_ptr();
        std::forward<Sender>(s).submit(on_value(
          [=] __host__ __device__ (value_type val) mutable {
            // TODO make this into a move if storage is not shared
            // TEMPORARY!!!
            *result_ptr = result_type(f(val));
            //new (result_ptr) result_type(f(val));
          }
        ));
      }

      return { *this, std::move(storage) };
    }

    // StreamSender, rvalue version: move the stream to the new value
    //   Note: this is inefficient because I don't think it always does what you want
    //   when working with adapters (i.e., this overload won't be found even
    //   if the underlying task that has been wrapped by some adapter is a 
    //   stream sender)
    template <typename StreamSender, typename Function>
    detail::unique_stream_task<
      cuda_stream_executor,
      detail::sender_result_t<StreamSender, Function const&>
    >
    make_value_task(StreamSender s, Function const& f,
      __REQUIRES(
        detail::is_stream_sender<StreamSender>::value
        && std::is_copy_constructible<Function>::value
        && not std::is_void<sender_value_t<StreamSender>>::value
      )
    ) {
      EXPECTS_NOT_NULL(stream_);

      using value_type = sender_value_t<std::decay_t<StreamSender>>;
      using result_type = decltype(f(std::declval<value_type>()));

      // make sender's stream wait on sender's event
      s.executor().wait_stream_event(s.event());
      CHECK_CUDA_ERROR(cudaStreamWaitEvent(s.executor().stream_, s.event(), 0));
      
      // create the storage for the result
      auto storage = detail::unique_value_storage<result_type>::make_storage();

      {
        // because cuda doesn't support init-capture yet...
        auto* result_ptr = storage.get_ptr();
        auto* value_ptr = s.storage_.get_ptr();
        submit(
          on_value(
            [=] __host__ __device__ (cuda_stream_executor subex) mutable {
              // TODO make this into a move if storage is not shared
              using storage_type = typename StreamSender::storage_type;
              //TEMPORARY FIX!!!
              *result_ptr = result_type(f(storage_type::get_from_pointer(value_ptr)));
              //new (result_ptr) result_type(f(storage_type::get_from_pointer(value_ptr)));
              storage_type::destroy_if_unique(value_ptr);
            }
          )
        );
      }

      return { *this, std::move(storage) };
    }

    template <class Receiver>
    __host__ __device__
    void submit(Receiver const& r) {
      // XXX is *this really the subexecutor we want?
      detail::_cuda_stream_executor_kernel<<<1,1,0,stream_>>>(r, *this);
      _post_submit_actions(r);
    }

    template <class Sender, class WrappedReceiver>
    __host__ __device__
    void submit(detail::_cuda_stream_task_receiver<Sender, WrappedReceiver>&& r)
    {
      detail::_cuda_stream_executor_kernel<<<1,1,0,stream_>>>(r, *this);
      _post_submit_actions(std::move(r));
    }

    template <class Sender>
    __host__ __device__
    void submit(detail::_cuda_stream_task_receiver<Sender, std::experimental::execution::sink_receiver>&& r)
    {
      _post_submit_actions(std::move(r));
    }

    auto require(std::experimental::execution::blocking_t::always_t) const {
      return cuda_stream_executor<std::experimental::execution::blocking_t::always_t>(*this);
    }
    auto require(std::experimental::execution::blocking_t::never_t) const {
      return cuda_stream_executor<std::experimental::execution::blocking_t::never_t>(*this);
    }
    auto require(std::experimental::execution::blocking_t::possibly_t) const {
      return cuda_stream_executor<std::experimental::execution::blocking_t::possibly_t>(*this);
    }
    static constexpr std::experimental::execution::sender_desc<std::exception_ptr, cuda_stream_executor>
    query(std::experimental::execution::sender_description_t) noexcept { return {}; }
    static constexpr void query(std::experimental::execution::sender_t) noexcept { }

    __host__ __device__
    auto executor() const noexcept { return *this; }
};
