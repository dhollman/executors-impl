#pragma once

namespace _on_value_impl {

// this is really a much more general utility
// TODO use the more general implementation like in Eric's code
template <typename F>
struct receiver {
  F f_;
  template <class... Args>
  __host__ __device__
  void set_value(Args&&... args) && {
    std::move(f_)(std::forward<Args>(args)...);
  }
  template <class E>
  __host__ __device__
  void set_error(E&& e) && {
    std::terminate();
  }
  __host__ __device__
  void set_done() && {
    std::terminate();
  }
  static constexpr void query(std::experimental::execution::receiver_t) noexcept { }
};

} // end namespace _on_value_impl

template <typename F>
inline auto on_value(F&& f) {
  return _on_value_impl::receiver<std::decay_t<F>>{std::forward<F>(f)};
}



//template <class Receiver, class Value>
//struct executor_receiver_with_value {
//  Receiver r;
//  Value v;
//
//  template <class Subexecutor>
//  __host__ __device__
//  void set_value(Subexecutor&&) && {
//    std::move(r).set_value(std::move(v));
//  }
//
//  template <class E>
//  __host__ __device__
//  void set_error(E&& e) && {
//    std::move(r).set_error(std::forward<E>(e));
//  }
//
//  __host__ __device__
//  void set_done() && {
//    std::move(r).set_done();
//  }
//
//};
//
//// This is effectively a "lifter"
//template <class Receiver, class Value>
//auto make_executor_receiver_with_value(Receiver&& r, Value&& v) {
//  return executor_receiver_with_value<std::decay_t<Receiver>, std::decay_t<Value>>{
//    std::forward<Receiver>(r), std::forward<Value>(v)
//  };
//}