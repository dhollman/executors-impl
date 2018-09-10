#pragma once

#include <experimental/execution>
#include <tuple>

namespace std {
namespace experimental {
inline namespace executors_v1 {
namespace execution {
namespace __default_join_handler_impl {

template <template <class...> class, class...>
struct __list_cat;

template <template <class...> class List, class Result>
struct __list_cat<List, Result>
{
  using type = Result;
};

template <template <class...> class List, class... A1, class... A2, class... Lists>
struct __list_cat<List, List<A1...>, List<A2...>, Lists...>
{
  using type = typename __list_cat<List, List<A1..., A2...>, Lists...>::type;
};

template <class Error, class... Senders>
struct __joined_on_submit_fn;

template <class FinalReceiver, class ValueTuple, class... Senders>
struct __nested_joined_recv;

template <class Error, class Sender, class... Senders>
struct __joined_on_submit_fn<Error, Sender, Senders...> {
  tuple<Sender, Senders...> senders_;
  struct sender_desc_t {
    using error_type = Error;
    template <template <class...> class List>
    using value_types = typename __list_cat<List,
      typename sender_traits<Sender>::template value_types<List>,
      typename sender_traits<Senders>::template value_types<List>...
    >::type;
  };

  template <Receiver R>
  void operator()(R&& r) {
    using nested_recv_t = __nested_joined_recv<decay_t<R>, tuple<>, Senders...>;
    std::apply(
      [&](auto&& sender, auto&&... senders) {
        forward<decltype(sender)>(sender).submit(
          nested_recv_t{
            forward<R>(r),
            std::tuple<>(),
            std::tuple<decay_t<decltype(senders)>...>(forward<decltype(senders)>(senders)...)
          }
        );
      },
      std::move(senders_)
    );
  }
  
};

template <class FinalReceiver>
struct __nested_joined_recv_common {

  FinalReceiver recv_;

  static constexpr void query(receiver_t) { }

  template <class E>
  void set_error(E&& e) {
    std::move(recv_).set_error(forward<E>(e));
  }

  void set_done() {
    std::move(recv_).set_done();
  }
};

// base case
template <class FinalReceiver, class... Values>
struct __nested_joined_recv<FinalReceiver, tuple<Values...>>
  : __nested_joined_recv_common<FinalReceiver>
{
  using base_t = __nested_joined_recv_common<FinalReceiver>;

  tuple<Values...> values_;

  template <
    class FinalRecvDeduced,
    class ValuesTupleDeduced
  >
  __nested_joined_recv(
    FinalRecvDeduced&& recv,
    ValuesTupleDeduced&& values,
    tuple<>
  ) : base_t{std::forward<FinalRecvDeduced>(recv)},
      values_(std::forward<ValuesTupleDeduced>(values))
  { }

  __nested_joined_recv(__nested_joined_recv const&) = default;
  __nested_joined_recv(__nested_joined_recv&&) = default;
  __nested_joined_recv& operator=(__nested_joined_recv const&) = default;
  __nested_joined_recv& operator=(__nested_joined_recv&&) = default;
  ~__nested_joined_recv() = default;

  template <class... InnerVals>
  void set_value(InnerVals&&... inner_vals) {
    std::apply(
      [&](auto&&... outer_vals) {
        std::move(this->recv_).set_value(
          forward<decltype(outer_vals)>(outer_vals)...,
          forward<InnerVals>(inner_vals)...
        );
      },
      std::move(values_)
    );
  }

};

// recursive case
template <class FinalReceiver, class... Values, class Sender, class... Senders>
struct __nested_joined_recv<FinalReceiver, tuple<Values...>, Sender, Senders...> 
  : __nested_joined_recv_common<FinalReceiver>
{
  using next_recv_t = __nested_joined_recv<
    FinalReceiver,
    typename __list_cat<tuple, tuple<Values...>, typename sender_traits<Sender>::template value_types<tuple>>::type,
    Senders...
  >;

  using base_t = __nested_joined_recv_common<FinalReceiver>;

  tuple<Values...> values_;
  tuple<Sender, Senders...> senders_;

  template <
    class FinalRecvDeduced,
    class ValuesTupleDeduced,
    class SendersTupleDeduced
  >
  __nested_joined_recv(
    FinalRecvDeduced&& recv,
    ValuesTupleDeduced&& values,
    SendersTupleDeduced&& senders
  ) : base_t{std::forward<FinalRecvDeduced>(recv)},
      values_(std::forward<ValuesTupleDeduced>(values)),
      senders_(std::forward<SendersTupleDeduced>(senders))
  { }

  __nested_joined_recv(__nested_joined_recv const&) = default;
  __nested_joined_recv(__nested_joined_recv&&) = default;
  __nested_joined_recv& operator=(__nested_joined_recv const&) = default;
  __nested_joined_recv& operator=(__nested_joined_recv&&) = default;
  ~__nested_joined_recv() = default;

  template <class... InnerVals>
  void set_value(InnerVals&&... inner_vals) {
    // TODO make this more readable
    std::apply(
      [&](auto&&... outer_vals) {
        std::apply(
          [&](auto&& sender, auto&&... rest_senders) {
            std::experimental::execution::submit(
              forward<decltype(sender)>(sender),
              next_recv_t(
                std::move(this->recv_),
                tuple<Values..., decay_t<InnerVals>...>{
                  forward<decltype(outer_vals)>(outer_vals)...,
                  forward<InnerVals>(inner_vals)...
                },
                tuple<Senders...>(forward<decltype(rest_senders)>(rest_senders)...)
              )
            );
          },
          std::move(senders_)
        );
      },
      std::move(values_)
    );
  }

};

template <class Executor, class Error>
class __impl {
private:

  Executor ex_;

public:

  explicit __impl(Executor ex) : ex_(ex) { }

  __impl(__impl const&) = default;
  __impl(__impl&&) = default;
  __impl& operator=(__impl const&) = default;
  __impl& operator=(__impl&&) = default;
  ~__impl() = default;
  
  template <class... Senders>
  auto make_joined_sender(Senders&&... s) const {
    return execution::sender(
      __joined_on_submit_fn<Error, decay_t<Senders>...>{
        tuple<decay_t<Senders>...>(forward<Senders>(s)...)
      },
      [ex_=ex_]() { return ex_; }
    );
  }

};

} // end namespace __default_join_handler_impl

template <typename E=exception_ptr>
struct join_handler_t : __property_base<false, false> {

  // TODO polymorphic query result type

  template <class Executor>
  friend auto query(Executor const& ex, join_handler_t) {
    return __default_join_handler_impl::__impl<Executor, E>{ex};
  }

};

template <class E=exception_ptr>
constexpr inline join_handler_t<E> join_handler;


} // end namespace execution
} // end namespace executors_v1
} // end namespace experimental
} // end namespace std