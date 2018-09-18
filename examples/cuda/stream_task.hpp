#pragma once

#include "cuda_stream_fwd.hpp"
#include "storage.hpp"
#include "event_lifetime.hpp"
#include "helpers.hpp"
#include "util.hpp"

#include <utility>
#include <type_traits>
#include <experimental/execution>

namespace detail {

template <typename Sender, typename WrappedReceiver>
struct _cuda_stream_task_receiver {
  WrappedReceiver r;
  Sender* sender_ptr; // Not expected to be valid on the device
  sender_value_t<Sender>* value_ptr; // a device pointer that will contain the value to send
                                         // by the time set_value() is called
  template <std::experimental::execution::SenderTo<WrappedReceiver> SubExecutor>
  __host__ __device__
  void set_value(SubExecutor&& subex) && {
    std::move(r).set_value(Sender::storage_type::get_from_pointer(std::move(value_ptr)));
    // it's been either moved or copied; if it's unique we still need to destroy it, though
    Sender::storage_type::destroy_if_unique(value_ptr);
  }

  static constexpr void query(std::experimental::execution::receiver_t) noexcept { }
};

template <class Executor, class T>
class shared_stream_task;


template <typename Executor, typename T>
class unique_stream_task
{
  private:

    using storage_type = unique_value_storage<T>;

    Executor executor_;
    storage_type storage_;
    unique_event event_;

    template <class, class>
    friend class unique_stream_task;
    template <class, class>
    friend class shared_stream_task;
    template <class>
    friend class ::cuda_stream_executor;
    template <class, class>
    friend struct _cuda_stream_task_receiver;

    unique_stream_task(Executor const& executor, storage_type storage)
      : executor_(executor),
        storage_(std::move(storage)),
        event_(executor.make_cuda_event())
    { 
      executor_.record_stream_event(event_.get());
    }

    // Used for via() specialization
    unique_stream_task(Executor const& executor, storage_type&& storage, unique_event&& event)
      : executor_(executor), storage_(std::move(storage)), event_(std::move(event))
    { }

    auto event() const { return event_.get(); }

  public:

    using value_type = T;


    unique_stream_task() = default;
    unique_stream_task(unique_stream_task const&) = delete;
    unique_stream_task(unique_stream_task&&) = default;
    unique_stream_task& operator=(unique_stream_task const&) = delete;
    unique_stream_task& operator=(unique_stream_task&&) = default;
    ~unique_stream_task() = default;


    template <typename Receiver>
    void submit(Receiver&& r) && {
      EXPECTS_NOT_NULL(event_.get());

      executor_.record_stream_event(event_.get());

      auto value_ptr = storage_.get_ptr();
      std::move(executor_).submit(
        _cuda_stream_task_receiver<unique_stream_task, Receiver>{std::forward<Receiver>(r), this, value_ptr}
      );

      // for rvalue version, release the event
      event_ = nullptr;
    }


    // Override the global customization point "via" only for executors we know about
    template <class... Properties>
    auto via(cuda_stream_executor<Properties...> ex) && {
      if(ex.stream_ != executor_.stream_) ex.record_stream_event(event());
      return unique_stream_task<cuda_stream_executor<Properties...>, T>(
        ex, std::move(storage_), std::move(event_)
      );
    }


    shared_stream_task<Executor, T>
    share() && { 
      return { std::move(*this) };
    }

    static constexpr std::experimental::execution::sender_desc<std::exception_ptr, T>
    query(std::experimental::execution::sender_description_t) noexcept { return {}; }
    static constexpr void query(std::experimental::execution::sender_t) noexcept { }

    // required for all Senders
    auto executor() const {
      return executor_;
    }
};

template <typename Executor, typename T>
class shared_stream_task
{
  private:

    using storage_type = shared_value_storage<T>;

    Executor executor_;
    storage_type storage_;
    reference_counted_event event_;

    shared_stream_task(
      unique_stream_task<Executor, T>&& other
    ) : executor_(std::move(other.executor_)),
        storage_(std::move(other.storage_)),
        event_(std::move(other.event_))
    { }

    // Used for via() specialization
    shared_stream_task(Executor const& executor, storage_type const& storage, reference_counted_event const& event)
      : executor_(executor), storage_(std::move(storage)), event_(std::move(event))
    { }

    // Used for via() specialization
    shared_stream_task(Executor const& executor, storage_type&& storage, reference_counted_event&& event)
      : executor_(executor), storage_(std::move(storage)), event_(std::move(event))
    { }

    template <class, class>
    friend class unique_stream_task;
    template <class, class>
    friend class shared_stream_task;
    template <class>
    friend class ::cuda_stream_executor;
    template <class, class>
    friend struct _cuda_stream_task_receiver;

    auto event() const { return event_.get(); }

  public:

    using value_type = T;


    shared_stream_task() = default;
    shared_stream_task(shared_stream_task&&) = default;
    shared_stream_task& operator=(shared_stream_task&&) = default;
    ~shared_stream_task() = default;


    // The copy constructor creates a parallel stream of execution
    shared_stream_task(shared_stream_task const& other)
      : executor_(
          // create a new executor to represent a new stream, with potentially concurrent work,
          // since that's what it means to copy a shared task
          other.executor_.make_analogous_executor_with_new_stream()
        ),
        storage_(other.storage_),
        event_(other.event_)
    {
      EXPECTS_NOT_NULL(other.event_.get());
      
      executor_.wait_stream_event(event_.get());
    }


    template <typename Receiver>
    void submit(Receiver&& r) & {
      EXPECTS_NOT_NULL(event());

      // any further calls to submit are potentially concurrent, so we need to create another stream
      auto replacement_executor = executor_.make_analogous_executor_with_new_stream();
      replacement_executor.record_stream_event(event());

      // swap the new executor into the member variable
      std::swap(replacement_executor, executor_);
      
      // use the old executor (after the swap) to handle the submission
      replacement_executor.wait_stream_event(event());

      // It's the last time the old executor will get used through this task, so move semantics are appropriate here
      auto value_ptr = storage_.get_ptr();
      std::move(replacement_executor).submit(
        _cuda_stream_task_receiver<shared_stream_task, std::decay_t<Receiver>>{std::forward<Receiver>(r), this, value_ptr}
      );
    }

    template <typename Receiver>
    void submit(Receiver&& r) && {
      EXPECTS_NOT_NULL(event_.get());

      executor_.record_stream_event(event_.get());

      auto value_ptr = storage_.get_ptr();
      std::move(executor_).submit(
        _cuda_stream_task_receiver<shared_stream_task, std::decay_t<Receiver>>{std::forward<Receiver>(r), this, value_ptr}
      );

      // for rvalue version, release the event
      event_ = nullptr;

    }

    // Override the global customization point "via" only for executors we know about
    template <class... Properties>
    auto via(cuda_stream_executor<Properties...> ex) && {
      if(ex.stream_ != executor_.stream_) ex.record_stream_event(event());
      return shared_stream_task<cuda_stream_executor<Properties...>, T>(
        std::move(ex), std::move(storage_), std::move(event_)
      );
    }

    // Override the global customization point "via" only for executors we know about
    template <class... Properties>
    auto via(cuda_stream_executor<Properties...> ex) const & {
      if(ex.stream_ != executor_.stream_) ex.record_stream_event(event());
      return shared_stream_task<cuda_stream_executor<Properties...>, T>(
        ex, storage_, event_
      );
    }

    // required for all Senders
    auto executor() const {
      return executor_;
    }

    static constexpr std::experimental::execution::sender_desc<std::exception_ptr, T>
    query(std::experimental::execution::sender_description_t) noexcept { return {}; }
    static constexpr void query(std::experimental::execution::sender_t) noexcept { }
};


template <typename T> struct is_stream_sender : std::false_type { };
template <typename... Args>
struct is_stream_sender<shared_stream_task<Args...>> : std::true_type { };
template <typename... Args>
struct is_stream_sender<unique_stream_task<Args...>> : std::true_type { };


template <class Sender, class Function>
using sender_result_t =
  decltype(std::declval<Function>()(std::declval<sender_value_t<std::decay_t<Sender>>>()));

} // end namespace detail