/*
//@HEADER
// ************************************************************************
//
//                      helpers.h
//                         DARMA
//              Copyright (C) 2018 Sandia Corporation
//
// Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact the DARMA developers (darma-admins@sandia.gov)
//
// ************************************************************************
//@HEADER
*/

#ifndef STD_EXPERIMENTAL_BITS_HELPERS_H
#define STD_EXPERIMENTAL_BITS_HELPERS_H

#include <utility>
#include <tuple>
#include <variant>

namespace std {
namespace experimental {
namespace executors_v1 {
namespace execution {

namespace helpers_impl {

template <class ValueCallable, class ErrorCallable, class TypeForVoid, class... Args>
class continuation_with_args_impl {
private:

  ValueCallable value_c_; // [[no_unique_address]]
  ErrorCallable error_c_; // [[no_unique_address]]
  tuple<Args...> args_; // [[no_unique_address]]

  template <class Callable, class Arg, size_t... Idxs>
  auto _do_call(Callable&& c, Arg&& v, index_sequence<Idxs...>) {
    return forward<Callable>(c)(
      forward<Arg>(v),
      std::get<Idxs>(std::move(args_))...
    );
  }

public:

  continuation_with_args_impl(continuation_with_args_impl&&) noexcept = default;

  template <
    typename ValueCallableDeduced,
    typename ErrorCallableDeduced,
    typename... ArgsDeduced
  >
  continuation_with_args_impl(
    ValueCallableDeduced&& vc,
    ErrorCallableDeduced&& ec,
    ArgsDeduced&&... args
  ) : value_c_(forward<ValueCallableDeduced>(vc)),
      error_c_(forward<ErrorCallableDeduced>(ec)),
      args_(forward<Args...>(args)...)
  { }

  auto value() && {
    return _do_call(std::move(value_c_), TypeForVoid{}, index_sequence_for<Args...>{});
  }
  template <class Value>
  auto value(Value&& v) && {
    return _do_call(std::move(value_c_), forward<Value>(v), index_sequence_for<Args...>{});
  }

  template <class Error>
  auto error(Error&& e) && {
    return _do_call(std::move(error_c_), forward<Error>(e), index_sequence_for<Args...>{});
  }

};

template <
  typename ValueCallableDeduced,
  typename ErrorCallableDeduced,
  typename... ArgsDeduced
>
continuation_with_args_impl(
  ValueCallableDeduced&& vc,
  ErrorCallableDeduced&& ec,
  ArgsDeduced&&... args
) -> continuation_with_args_impl<
  decay_t<ValueCallableDeduced>, decay_t<ErrorCallableDeduced>, decay_t<ArgsDeduced>...
>;

template <typename F>
struct on_void {
  F f;
  auto value() && {
    return std::move(f)();
  }
  template <typename E>
  auto error(E&& e) && {
    return std::forward<E>(e);
  }
};
template <typename F>
on_void(F f) -> on_void<F>;


template <typename F>
struct on_value {
  F f;
  auto value() && {
    return std::move(f)(std::monostate{});
  }
  template <typename Value>
  auto value(Value&& v) && {
    return std::move(f)(std::forward<Value>(v));
  }
  template <typename E>
  auto error(E&& e) && {
    return std::forward<E>(e);
  }
};
template <typename F>
on_value(F f) -> on_value<F>;

template <typename F>
struct on_error {
  F f;
  auto value() && {
    return;
  }
  template <typename Value>
  auto value(Value&& v) && {
    return std::forward<Value>(v);
  }
  template <typename E>
  auto error(E&& e) && {
    return std::move(f)(std::forward<E>(e));
  }
};
template <typename F>
on_error(F f) -> on_error<F>;

} // end namespace helpers_impl

template <class TypeForVoid=monostate, class ValueCallable, class ErrorCallable, class... Args>
inline auto make_continuation(
  ValueCallable&& vc, ErrorCallable&& ec, Args&&... args
) {
  return helpers_impl::continuation_with_args_impl<
    decay_t<ValueCallable>, decay_t<ErrorCallable>, TypeForVoid, decay_t<Args>...
  >(
    forward<ValueCallable>(vc), forward<ErrorCallable>(ec), forward<Args>(args)...
  );
}

template <class NullaryCallable>
inline auto on_void(NullaryCallable&& nc) {
  return helpers_impl::on_void{std::forward<NullaryCallable>(nc)};
}

template <class UnaryCallable>
inline auto on_value(UnaryCallable&& nc) {
  return helpers_impl::on_value{std::forward<UnaryCallable>(nc)};
}

template <class UnaryCallable>
inline auto on_error(UnaryCallable&& nc) {
  return helpers_impl::on_error{std::forward<UnaryCallable>(nc)};
}

} // end namespace execution
} // end namespace executors_v1
} // end namespace experimental
} // end namespace std

#endif //STD_EXPERIMENTAL_BITS_HELPERS_H
