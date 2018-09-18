#pragma once

#include "util.hpp"

#include <memory>

namespace detail {

struct event_destroy_deleter {
  void operator()(cudaEvent_t event) const {
    CHECK_CUDA_ERROR(cudaEventDestroy(event));
  }
};

class reference_counted_event;

class unique_event {
  private:

    // This object itself should only ever be on the CPU, so this is fine
    std::unique_ptr<std::remove_pointer_t<cudaEvent_t>, event_destroy_deleter> impl_;

    friend class reference_counted_event;

  public:

    unique_event() = default;
    unique_event(unique_event const&) = delete;
    unique_event(unique_event&&) noexcept = default;
    unique_event& operator=(unique_event const&) = delete;
    unique_event& operator=(unique_event&&) noexcept = default;
    ~unique_event() = default;

    unique_event& operator=(std::nullptr_t) {
      impl_ = nullptr;
      return *this;
    }

    explicit unique_event(cudaEvent_t event)
      : impl_(event)
    { }

    inline auto get() const noexcept {
      return impl_.get();
    }
};

class reference_counted_event {
  private:

    // This object itself should only ever be on the CPU, so this is fine
    std::shared_ptr<std::remove_pointer_t<cudaEvent_t>> impl_;

  public:

    reference_counted_event() = default;
    reference_counted_event(reference_counted_event const&) = default;
    reference_counted_event(reference_counted_event&&) noexcept = default;
    reference_counted_event& operator=(reference_counted_event const&) = default;
    reference_counted_event& operator=(reference_counted_event&&) noexcept = default;
    ~reference_counted_event() = default;

    reference_counted_event& operator=(std::nullptr_t) {
      impl_ = nullptr;
      return *this;
    }

    explicit reference_counted_event(cudaEvent_t event)
      : impl_(event, event_destroy_deleter{})
    { }

    reference_counted_event(unique_event&& other)
      : impl_(std::move(other.impl_))
    { }

    inline auto get() const noexcept {
      return impl_.get();
    }
};


} // end namespace detail