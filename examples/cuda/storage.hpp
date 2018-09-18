#pragma once

#include "util.hpp"

// Hack for now: this must be present to work with eager
template <typename S>
using sender_value_t = typename S::value_type;

namespace detail {

template <typename T>
struct unique_value_storage;

// TODO figure out when to call the destructor for shared storage
template<class T>
struct shared_value_storage {
  using value_type = T;

  struct control_block {
    T* result_ptr_;
    int32_t use_count_ = 0;
  };


  __host__ __device__
  constexpr shared_value_storage() : cb_(nullptr) { }
  __host__ __device__
  shared_value_storage(shared_value_storage const& other)
    : cb_(other.cb_)
  { 
    if(cb_) __sync_fetch_and_add(&cb_->use_count_, 1);
  }

  __host__ __device__
  shared_value_storage(shared_value_storage&& other)
    : cb_(other.cb_)
  { 
    if(&other != this) other.cb_ = nullptr;
  }

  __host__ __device__
  shared_value_storage(unique_value_storage<T>&& other) 
    : cb_(new control_block{other.result_ptr_, 1})
  {
    other.result_ptr_ = nullptr;
  }

  static auto make_storage() {
    auto result = shared_value_storage();
    result.cb_ = new control_block{nullptr, 1};
    CHECK_CUDA_ERROR(cudaMallocManaged(&(result.cb_->result_ptr_), sizeof(T)));
    return result;
  }

  ~shared_value_storage() {
    if(cb_) {
      auto old_count = __sync_fetch_and_add(&cb_->use_count_, -1);
      if(old_count == 1) {
        auto ptr = cb_->result_ptr_;
        delete cb_;
        CHECK_CUDA_ERROR(cudaFree(ptr));
      }
    }
  }

  __host__ __device__
  static T const& get_from_pointer(T* ptr) { return *ptr; }

  __host__ __device__
  static void destroy_if_unique(T* ptr) { }

  __host__ __device__
  T*& get_ptr() { return cb_->result_ptr_; }

  control_block* cb_ = nullptr;
};

template<class T>
struct unique_value_storage {

  using value_type = T;

  constexpr unique_value_storage() = default;
  unique_value_storage(unique_value_storage const& other) = delete;
  unique_value_storage& operator=(unique_value_storage const&) = delete;

  __host__ __device__
  unique_value_storage(unique_value_storage&& other) noexcept
    : result_ptr_(other.result_ptr_)
  {
    if(&other != this) other.result_ptr_ = nullptr;
  }

  __host__ __device__
  unique_value_storage& operator=(unique_value_storage&& other) noexcept
  {
    result_ptr_ = other.result_ptr_;
    if(&other != this) other.result_ptr_ = nullptr;
  }

  static auto make_storage() {
    auto result = unique_value_storage{};
    CHECK_CUDA_ERROR(cudaMallocManaged(&(result.result_ptr_), sizeof(T)));
    ENSURES_NOT_NULL(result.result_ptr_);
    return result;
  }

  ~unique_value_storage() {
    if(result_ptr_) {
      CHECK_CUDA_ERROR(cudaFree(result_ptr_));
    }
  }
  
  __host__ __device__
  static T&& get_from_pointer(T* ptr) { return std::move(*ptr); }

  __host__ __device__
  static void destroy_if_unique(T* ptr) { ptr->~T(); }

  __host__ __device__
  T*& get_ptr() noexcept { return result_ptr_; }

  T* result_ptr_ = nullptr;
};


} // end namespace detail