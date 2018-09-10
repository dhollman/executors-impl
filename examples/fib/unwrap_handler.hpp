
#pragma once

#include <experimental/execution>

namespace std {
namespace experimental {
inline namespace executors_v1 {
namespace execution {
namespace __default_unwrap_handler_impl {

template <class Executor>
class __impl {
private:
  template <class WrappedSender>
  struct __wrapped_sender {

    template <typename Recv>
    struct __receiver {
      Recv recv_;
      
      template <class NestedSender>
      void set_value(NestedSender&& s) && {
        forward<NestedSender>(s).submit(std::move(recv_));
      }

      template <class Error>
      void set_error(Error&& e) && {
        std::move(recv_).set_error(forward<Error>(e));
      }

      void set_done() && {
        std::move(recv_).set_done();
      }

      static constexpr void query(receiver_t) { }
    };

    template <std::Same<WrappedSender> WS>
    using _lazy_sender_desc_t = sender_traits<tuple_element_t<0,
      typename sender_traits<WS>::template value_types<tuple>
    >>;

    // be lazy with sender description retrieval, so it gets used only if needed
    template <std::Same<WrappedSender> WS>
    friend constexpr _lazy_sender_desc_t<WS> query(__wrapped_sender<WS> const&, sender_t) {
      return {};
    }

    WrappedSender wrapped_sender_;
    Executor ex_;

    template <Receiver R>
    void submit(R&& r) {
      std::move(wrapped_sender_).submit(
        __receiver<decay_t<R>>{forward<R>(r)}
      );
    }

    constexpr auto executor() const { return ex_; }
    
  };

  Executor ex_;

public:
  explicit __impl(Executor const& ex) : ex_(ex) { }

  __impl(__impl const&) = default;
  __impl(__impl&&) = default;
  __impl& operator=(__impl const&) = default;
  __impl& operator=(__impl&&) = default;
  ~__impl() = default;

  template <class Sender>
  auto make_unwrapped_sender(Sender&& s) {
    return __wrapped_sender<decay_t<Sender>>{forward<Sender>(s), ex_};
  }

};

} // end namespace __default_unwrap_handler_impl

struct unwrap_handler_t : __property_base<false, false> {

  // TODO polymorphic query result type

  template <class Executor>
  friend auto query(Executor const& ex, unwrap_handler_t) {
    return __default_unwrap_handler_impl::__impl<Executor>{ex};
  }

};

constexpr inline unwrap_handler_t unwrap_handler { };


} // end namespace execution
} // end namespace executors_v1
} // end namespace experimental
} // end namespace std