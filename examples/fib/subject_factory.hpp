
#pragma once

#include <utility>
#include <optional>
#include <mutex>
#include <vector>
#include <iostream>
#include <tuple>


namespace std {
namespace experimental {
inline namespace executors_v1 {
namespace execution {

namespace __default_subject_factory_impl {

template <class T, class E,  class Executor>
class __impl {
private:
  using mutex_t = std::mutex;
  class __subject {
  private:
    struct __subject_data {
      // This should really be a variant...
      optional<T> value_;
      optional<E> error_ = nullptr;
      bool done_ = false;
      // TODO mutex property
      mutex_t mtx_;
    };
    shared_ptr<__subject_data> data_;
    Executor ex_;
    any_receiver<E, T> recv_;    

    template <class R>
    void _execute_value(R&& r) {
      ex_.execute([recv=(R&&)r, val_=std::move(*data_->value_)]() mutable {
        std::experimental::execution::set_value(std::move(recv), std::move(val_));
      });
    }

    template <class R>
    void _execute_error(R&& r) {
      ex_.execute([recv=(R&&)r, err_=std::move(*data_->error_)]() mutable {
        std::move(recv).set_error(std::move(err_));
      });
    }

    template <class R>
    void _execute_done(R&& r) {
      ex_.execute([recv=forward<R>(r)]() mutable {
        std::move(recv).set_done();
      });
    }


  public:
    explicit __subject(Executor ex)
      : ex_(std::move(ex)),
        data_(make_shared<__subject_data>())
    { }
    __subject(__subject&&) noexcept = default;
    __subject(__subject const&) = default;
    __subject& operator=(__subject&&) noexcept = default;
    __subject& operator=(__subject const&) = default;

    // Receiver parts
    static constexpr void query(execution::receiver_t) { }

    template <typename U>
    void set_value(U&& val) && {
      assert(data_); // TODO make this into a contract-like thing
      std::lock_guard _lg{data_->mtx_};
      // ensure the constructor runs in the same place regardless
      data_->value_ = optional<T>(forward<U>(val));
      if(recv_) _execute_value(std::move(recv_));
    }
    template <typename EE>
    void set_error(EE&& err) {
      assert(data_); // TODO make this into a contract-like thing
      std::lock_guard _lg{data_->mtx_};
      // ensure the constructor runs in the same place regardless
      data_->error_ = optional<E>(forward<EE>(err));
      if(recv_) _execute_error(std::move(recv_));
    }
    void set_done() {
      assert(data_); // TODO make this into a contract-like thing
      std::lock_guard _lg{data_->mtx_};
      if(recv_) _execute_done(std::move(recv_));
    }

    // Sender parts
    static constexpr execution::sender_desc<E, T> query(execution::sender_t) { return {}; }

    template <ReceiverOf<E, T> R>
    void submit(R&& r) && {
      assert(data_); // TODO make this into a contract-like thing
      std::lock_guard _lg{data_->mtx_};
      if(data_->value_) _execute_value(forward<R>(r));
      else if(data_->error_) _execute_error(forward<R>(r));
      else if(data_->done_) _execute_done(forward<R>(r));
      else recv_ = any_receiver<E, T>(forward<R>(r));
    }

    auto executor() const { return ex_; }
      
  };

  Executor ex_;

public:

  explicit __impl(Executor ex) : ex_(ex) { }

  __impl(__impl const&) = default;
  __impl(__impl&&) = default;
  __impl& operator=(__impl const&) = default;
  __impl& operator=(__impl&&) = default;
  ~__impl() = default;

  auto make_subject() const {
    return __subject{ex_};
  }
};

} // end namespace __default_subject_factory_impl

template <typename T, typename E=exception_ptr>
struct subject_factory_t : __property_base<false, false> {
  // TODO polymorphic result type


  template <class Executor>
  friend auto query(Executor const& ex, subject_factory_t) {
    return __default_subject_factory_impl::__impl<T, E, Executor>{ex};
  }

};

template <class T, class E=exception_ptr>
constexpr inline subject_factory_t<T, E> subject_factory;


} // end namespace execution
} // end namespace executors_v1
} // end namespace experimental
} // end namespace std