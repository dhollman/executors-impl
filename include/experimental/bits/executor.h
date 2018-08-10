#ifndef STD_EXPERIMENTAL_BITS_EXECUTOR_H
#define STD_EXPERIMENTAL_BITS_EXECUTOR_H

#include <experimental/bits/bad_executor.h>
#include <experimental/bits/executor_value.h>
#include <experimental/bits/executor_error.h>

#include <experimental/execution>

#include <atomic>
#include <cassert>
#include <memory>
#include <utility>

namespace std {
namespace experimental {
inline namespace executors_v1 {
namespace execution {
namespace executor_impl {


//==============================================================================
// <editor-fold desc="property_list and helpers"> {{{1

template<class... SupportableProperties>
struct property_list;

template<>
struct property_list<> {};

template<class Head, class... Tail>
struct property_list<Head, Tail...> {};

template<class Property, class... SupportableProperties>
struct contains_exact_property;

template<class Property, class Head, class... Tail>
struct contains_exact_property<Property, Head, Tail...>
  : std::conditional<std::is_same<Property, Head>::value, std::true_type, contains_exact_property<Property, Tail...>>::type {};

template<class Property>
struct contains_exact_property<Property> : std::false_type {};

template<class Property, class... SupportableProperties>
static constexpr bool contains_exact_property_v = contains_exact_property<Property, SupportableProperties...>::value;

template<class Property, class... SupportableProperties>
struct contains_convertible_property;

template<class Property, class Head, class... Tail>
struct contains_convertible_property<Property, Head, Tail...>
  : std::conditional<std::is_convertible<Property, Head>::value, std::true_type, contains_convertible_property<Property, Tail...>>::type {};

template<class Property>
struct contains_convertible_property<Property> : std::false_type {};

template<class Property, class... SupportableProperties>
static constexpr bool contains_convertible_property_v = contains_convertible_property<Property, SupportableProperties...>::value;

template<class Property, class... SupportableProperties>
struct find_convertible_property;

template<class Property, class Head, class... Tail>
struct find_convertible_property<Property, Head, Tail...>
  : std::conditional<std::is_convertible<Property, Head>::value, std::decay<Head>, find_convertible_property<Property, Tail...>>::type {};

template<class Property>
struct find_convertible_property<Property> {};

template<class Property, class... SupportableProperties>
using find_convertible_property_t = typename find_convertible_property<Property, SupportableProperties...>::type;

template<class PropertyList, class... SupportableProperties>
struct contains_exact_property_list;

template<class Head, class... Tail, class... SupportableProperties>
struct contains_exact_property_list<property_list<Head, Tail...>, SupportableProperties...>
  : std::integral_constant<bool, contains_exact_property_v<Head, SupportableProperties...>
      && contains_exact_property_list<property_list<Tail...>, SupportableProperties...>::value> {};

template<class... SupportableProperties>
struct contains_exact_property_list<property_list<>, SupportableProperties...> : std::true_type {};

template<class PropertyList, class... SupportableProperties>
static constexpr bool contains_exact_property_list_v = contains_exact_property_list<PropertyList, SupportableProperties...>::value;

struct identity_property
{
  static constexpr bool is_requirable = true;
  static constexpr bool is_preferable = true;
  template<class Executor> static constexpr bool static_query_v = true;
  static constexpr bool value() { return true; }
};

template<class Property, class... SupportableProperties>
using conditional_property_t = typename conditional<
  contains_exact_property_v<Property, SupportableProperties...>,
  Property, identity_property>::type;

// </editor-fold> end property_list and helpers }}}1
//==============================================================================


//==============================================================================
// <editor-fold desc="is_valid_target"> {{{1

template<class Executor, class T, class E, class... SupportableProperties>
struct is_valid_target;

// This is going into 20, but for now:
template <typename T>
struct _type_identity { using type = T; };

// TODO decide if this should be convertible instead of same
template<class Executor, class T, class E>
struct valid_target_types_match {
  using type = std::bool_constant<
    is_same_v<executor_value_t<Executor>, T>
      && is_same_v<executor_error_t<Executor>, E>
  >;
};

template<class Executor, class T, class E>
struct is_valid_target<Executor, T, E>
  : conditional_t<
      // prevent calling executor_value_t on a non-executor
      is_executor_v<Executor, T, E>,
      valid_target_types_match<Executor, T, E>,
      std::false_type
    >::type
{ };

template<class Executor, class T, class E, class Head, class... Tail>
struct is_valid_target<Executor, T, E, Head, Tail...>
  : std::integral_constant<bool,
      (!Head::is_requirable || can_require_v<Executor, Head>)
      && (!Head::is_preferable || can_prefer_v<Executor, Head>)
      && (Head::is_requirable || Head::is_preferable || can_query_v<Executor, Head>)
      && is_valid_target<Executor, T, E, Tail...>::value> {};

template<class Executor, class T, class E, class... SupportableProperties>
static constexpr bool is_valid_target_v = is_valid_target<Executor, T, E, SupportableProperties...>::value;

// </editor-fold> end is_valid_target }}}1
//==============================================================================


//==============================================================================
// <editor-fold desc="polymorphic wrappers for functions and continuations"> {{{1

template<class R, class... Args>
struct single_use_func_base
{
  virtual ~single_use_func_base() {}
  virtual R call(Args...) = 0;
};

template<class Function, class R, class... Args>
struct single_use_func : single_use_func_base<R, Args...>
{
  Function function_;

  explicit single_use_func(Function f) : function_(std::move(f)) {}

  virtual R call(Args... args)
  {
    std::unique_ptr<single_use_func> fp(this);
    Function f(std::move(function_));
    fp.reset();
    return f(std::forward<Args>(args)...);
  }
};

template<class R, class... Args>
struct multi_use_func_base
{
  virtual ~multi_use_func_base() {}
  virtual R call(Args...) = 0;
};

template<class Function, class R, class... Args>
struct multi_use_func : multi_use_func_base<R, Args...>
{
  Function function_;

  explicit multi_use_func(Function f) : function_(std::move(f)) {}

  virtual R call(Args... args)
  {
    return function_(std::forward<Args>(args)...);
  }
};

using shared_factory_base = multi_use_func_base<std::shared_ptr<void>>;
template<class SharedFactory> using shared_factory = multi_use_func<SharedFactory, std::shared_ptr<void>>;

using bulk_func_base = multi_use_func_base<void, std::size_t, std::shared_ptr<void>&>;
template<class Function> using bulk_func = multi_use_func<Function, void, std::size_t, std::shared_ptr<void>&>;

using twoway_func_base = single_use_func_base<std::shared_ptr<void>>;
template<class Function> using twoway_func = single_use_func<Function, std::shared_ptr<void>>;

using twoway_then_func_base = single_use_func_base<void, std::shared_ptr<void>, std::exception_ptr>;
template<class Function> using twoway_then_func = single_use_func<Function, void, std::shared_ptr<void>, std::exception_ptr>;

template <class T, class E, class R, class S>
struct single_use_continuation_base {
  // for now, omit distinction between T&& and T const& versions for simplicity
  virtual R value(T) && = 0;
  virtual S error(E) && = 0;
  virtual ~single_use_continuation_base() noexcept = default;
};
template <class E, class R, class S>
struct single_use_continuation_base<void, E, R, S> {
  virtual R value() && = 0;
  virtual S error(E) && = 0;
  virtual ~single_use_continuation_base() noexcept = default;
};

template <class Continuation, class T, class E, class R, class S>
struct single_use_continuation
  : single_use_continuation_base<T, E, R, S>
{
  private:
    Continuation c_;

  public:

    explicit single_use_continuation(Continuation&& c) : c_(std::move(c)) { }

    R value(T v) && override {
      return std::move(c_).value(std::move(v));
    }

    S error(E e) && override {
      return std::move(c_).error(std::move(e));
    }
};

template <class Continuation, class E, class R, class S>
struct single_use_continuation<Continuation, void, E, R, S>
  : single_use_continuation_base<void, E, R, S>
{
  private:
    Continuation c_;

  public:

    explicit single_use_continuation(Continuation&& c) : c_(std::move(c)) { }

    R value() && override {
      return std::move(c_).value();
    }

    S error(E e) && override {
      return std::move(c_).error(std::move(e));
    }
};

// </editor-fold> end polymorphic wrappers for functions and continuations }}}1
//==============================================================================


//==============================================================================
// <editor-fold desc="private implementation class for polymorphic executor"> {{{1

template <class T, class E>
struct impl_base
{
  virtual ~impl_base() {}
  virtual void execute(unique_ptr<single_use_continuation_base<T, E, void, E>> c) && = 0;

  virtual const type_info& target_type() const = 0;
  virtual void* target() = 0;
  virtual const void* target() const = 0;
  virtual bool equals(const impl_base* e) const noexcept = 0;
  virtual impl_base* require(const type_info&, const void* p) && = 0;
  virtual impl_base* prefer(const type_info&, const void* p) const = 0;
  virtual void* query(const type_info&, const void* p) const = 0;
};

template<class Executor, class... SupportableProperties>
struct impl : impl_base<executor_value_t<Executor>, executor_error_t<Executor>>
{
  Executor executor_;

  using base_t = impl_base<executor_value_t<Executor>, executor_error_t<Executor>>;
  using value_t = executor_value_t<Executor>;
  using error_t = executor_error_t<Executor>;
  using execute_continuation_t = single_use_continuation_base<
    value_t, error_t, void, error_t
  >;

  explicit impl(Executor ex) : executor_(std::move(ex)) {}

  void
  execute(unique_ptr<execute_continuation_t> c) && override {
    std::move(executor_).execute(std::move(*c));
  }

  virtual const type_info& target_type() const
  {
    return typeid(executor_);
  }

  virtual void* target()
  {
    return &executor_;
  }

  virtual const void* target() const
  {
    return &executor_;
  }

  virtual bool equals(const base_t* e) const noexcept
  {
    if (this == e)
      return true;
    if (target_type() != e->target_type())
      return false;
    return executor_ == *static_cast<const Executor*>(e->target());
  }

  base_t* require_helper(property_list<>, const type_info&, const void*) &&
  {
    std::abort(); // should be unreachable
    return nullptr;
  }

  template<class Head, class... Tail>
  base_t* require_helper(property_list<Head, Tail...>, const type_info& t, const void* p, typename std::enable_if<Head::is_requirable>::type* = 0) &&
  {
    if (t == typeid(Head))
    {
      using executor_type = decltype(execution::require(std::move(executor_), *static_cast<const Head*>(p)));
      return new impl<executor_type, SupportableProperties...>(execution::require(std::move(executor_), *static_cast<const Head*>(p)));
    }
    return std::move(*this).require_helper(property_list<Tail...>{}, t, p);
  }

  template<class Head, class... Tail>
  base_t* require_helper(property_list<Head, Tail...>, const type_info& t, const void* p, typename std::enable_if<!Head::is_requirable>::type* = 0) &&
  {
    return std::move(*this).require_helper(property_list<Tail...>{}, t, p);
  }

  base_t* require(const type_info& t, const void* p) && override
  {
    return std::move(*this).require_helper(property_list<SupportableProperties...>{}, t, p);
  }

  base_t* prefer_helper(property_list<>, const type_info&, const void*) &&
  {
    // probably should be something like &std::move(*this) for linters, but I'm not doing that...
    return this;
  }

  template<class Head, class... Tail>
  base_t* prefer_helper(property_list<Head, Tail...>, const type_info& t, const void* p, typename std::enable_if<Head::is_preferable>::type* = 0) &&
  {
    if (t == typeid(Head))
    {
      using executor_type = decltype(execution::prefer(executor_, *static_cast<const Head*>(p)));
      return new impl<executor_type, SupportableProperties...>(execution::prefer(std::move(executor_), *static_cast<const Head*>(p)));
    }
    return std::move(*this).prefer_helper(property_list<Tail...>{}, t, p);
  }

  template<class Head, class... Tail>
  base_t* prefer_helper(property_list<Head, Tail...>, const type_info& t, const void* p, typename std::enable_if<!Head::is_preferable>::type* = 0) &&
  {
    return std::move(*this).prefer_helper(property_list<Tail...>{}, t, p);
  }

  virtual base_t* prefer(const type_info& t, const void* p) &&
  {
    return std::move(*this).prefer_helper(property_list<SupportableProperties...>{}, t, p);
  }

  void* query_helper(property_list<>, const type_info&, const void*) const
  {
    return nullptr;
  }

  template<class Head, class... Tail>
  void* query_helper(property_list<Head, Tail...>, const type_info& t, const void* p, typename std::enable_if<can_query_v<Executor, Head>>::type* = 0) const
  {
    if (t == typeid(Head))
      return new std::tuple<typename Head::template polymorphic_query_result_type<value_t, error_t, SupportableProperties...>>(execution::query(executor_, *static_cast<const Head*>(p)));
    return query_helper(property_list<Tail...>{}, t, p);
  }

  template<class Head, class... Tail>
  void* query_helper(property_list<Head, Tail...>, const type_info& t, const void* p, typename std::enable_if<!can_query_v<Executor, Head>>::type* = 0) const
  {
    return query_helper(property_list<Tail...>{}, t, p);
  }

  virtual void* query(const type_info& t, const void* p) const
  {
    return this->query_helper(property_list<SupportableProperties...>{}, t, p);
  }
};

// </editor-fold> end private implementation class for polymorphic executor }}}1
//==============================================================================

} // namespace executor_impl

// TODO write a shared_executor wrapper also?
template<class T, class E, class... SupportableProperties>
class executor
{
  using polymorphic_executor_continuation_t = executor_impl::single_use_continuation_base<
    T, E, void, E
  >;
public:

  using value_type = T;
  using error_type = E;

  // construct / copy / destroy:

  executor() noexcept
    : impl_(nullptr)
  {
  }

  executor(std::nullptr_t) noexcept
    : impl_(nullptr)
  {
  }

  executor(const executor& e) = delete;

  executor(executor&& e) noexcept
    : impl_(e.impl_)
  {
    e.impl_ = nullptr;
  }

  template<class Executor>
  executor(Executor e,
      typename std::enable_if<executor_impl::is_valid_target_v<
        Executor, T, E, SupportableProperties...
      >>::type* = 0
  )
  {
    impl_ = std::make_unique<executor_impl::impl<Executor, SupportableProperties...>>(std::move(e));
  }

  // move-from-compatible ctor
  template<class... OtherSupportableProperties>
  executor(executor<T, E, OtherSupportableProperties...>&& e,
      typename std::enable_if<executor_impl::contains_exact_property_list_v<
        executor_impl::property_list<SupportableProperties...>,
          OtherSupportableProperties...>>::type* = 0) noexcept
    : impl_(std::move(e.impl_))
  {
    e.impl_ = nullptr;
  }

  template<class... OtherSupportableProperties>
  executor(executor<T, E, OtherSupportableProperties...>&& e,
      typename std::enable_if<!executor_impl::contains_exact_property_list_v<
        executor_impl::property_list<SupportableProperties...>,
          OtherSupportableProperties...>>::type* = 0) = delete;

  executor& operator=(const executor& e) noexcept = delete;

  executor& operator=(executor&& e) noexcept = default;

  executor& operator=(nullptr_t) noexcept
  {
    if (impl_) impl_->destroy();
    impl_ = nullptr;
    return *this;
  }

  template<class Executor> executor& operator=(Executor e)
  {
    return operator=(executor(std::move(e)));
  }

  ~executor() noexcept = default;

  // polymorphic executor modifiers:

  void swap(executor& other) noexcept
  {
    std::swap(impl_, other.impl_);
  }

  template<class Executor> void assign(Executor e)
  {
    operator=(executor(std::move(e)));
  }

  // executor operations:

  template<class Property,
    class = typename std::enable_if<
      executor_impl::find_convertible_property_t<Property, SupportableProperties...>::is_requirable>::type>
  executor require(const Property& p) &&
  {
    executor_impl::find_convertible_property_t<Property, SupportableProperties...> p1(p);
    impl_ = impl_ ? std::move(*impl_).require(typeid(p1), &p1) : throw bad_executor();
    return std::move(impl_);
  }

  template<class Property,
    class = typename std::enable_if<
      executor_impl::find_convertible_property_t<Property, SupportableProperties...>::is_preferable>::type>
  friend executor prefer(executor&& e, const Property& p)
  {
    executor_impl::find_convertible_property_t<Property, SupportableProperties...> p1(p);
    e.impl_ = e.get_impl() ? std::move(*e.impl_).prefer(typeid(p1), &p1) : throw bad_executor();
    return std::move(e.impl_);
  }

  template<class Property>
  auto query(const Property& p) const
    -> typename std::enable_if<
      executor_impl::find_convertible_property_t<Property, SupportableProperties...>::is_requirable
        || executor_impl::find_convertible_property_t<Property, SupportableProperties...>::is_preferable,
      typename executor_impl::find_convertible_property_t<Property, SupportableProperties...>::template polymorphic_query_result_type<T, E, SupportableProperties...>>::type
  {
    executor_impl::find_convertible_property_t<Property, SupportableProperties...> p1(p);
    using result_type = typename decltype(p1)::template polymorphic_query_result_type<T, E, SupportableProperties...>;
    using tuple_type = std::tuple<result_type>;
    if (!impl_) throw bad_executor();
    std::unique_ptr<tuple_type> result(static_cast<tuple_type*>(impl_->query(typeid(p1), &p1)));
    return result ? std::get<0>(*result) : result_type();
  }

  template<class Property>
  auto query(const Property& p) const
    -> typename std::enable_if<
      !executor_impl::find_convertible_property_t<Property, SupportableProperties...>::is_requirable
        && !executor_impl::find_convertible_property_t<Property, SupportableProperties...>::is_preferable,
      typename executor_impl::find_convertible_property_t<Property, SupportableProperties...>::template polymorphic_query_result_type<T, E, SupportableProperties...>>::type
  {
    executor_impl::find_convertible_property_t<Property, SupportableProperties...> p1(p);
    using result_type = typename decltype(p1)::template polymorphic_query_result_type<T, E, SupportableProperties...>;
    using tuple_type = std::tuple<result_type>;
    if (!impl_) throw bad_executor();
    std::unique_ptr<tuple_type> result(static_cast<tuple_type*>(impl_->query(typeid(p1), &p1)));
    return std::get<0>(*result);
  }

  template<class Continuation>
  void execute(Continuation c) &&
  {
    using continuation_impl_t = executor_impl::single_use_continuation<Continuation, T, E, void, E>;
    std::unique_ptr<polymorphic_executor_continuation_t> cp(
      new continuation_impl_t(std::move(c))
    );
    impl_ ? std::move(*impl_).execute(std::move(cp)) : throw bad_executor();
  }

  // polymorphic executor capacity:

  explicit operator bool() const noexcept
  {
    return !!impl_;
  }

  // polymorphic executor target access:

  const type_info& target_type() const noexcept
  {
    return impl_ ? impl_->target_type() : typeid(void);
  }

  template<class Executor> Executor* target() noexcept
  {
    return impl_ ? static_cast<Executor*>(impl_->target()) : nullptr;
  }

  template<class Executor> const Executor* target() const noexcept
  {
    return impl_ ? static_cast<Executor*>(impl_->target()) : nullptr;
  }

  // polymorphic executor comparisons:

  friend auto operator==(const executor& a, const executor& b) noexcept
  {
    if (!a.get_impl() && !b.get_impl())
      return true;
    if (a.get_impl() && b.get_impl())
      return a.get_impl()->equals(b.get_impl());
    return false;
  }

  template <typename... OtherSupportableProperties>
  friend auto operator==(const executor& a, const executor<T, E, OtherSupportableProperties...>& b) noexcept
    -> enable_if_t<
         !is_same_v<
           executor_impl::property_list<OtherSupportableProperties...>,
           executor_impl::property_list<SupportableProperties...>
         >,
         bool
       >
  {
    if (!a.get_impl() && !executor::get_other_impl(b))
      return true;
    if (a.get_impl() && executor::get_other_impl(b))
      return a.get_impl()->equals(executor::get_other_impl(b));
    return false;
  }

  template <typename... OtherSupportableProperties>
  friend auto operator!=(const executor& a, const executor<T, E, OtherSupportableProperties...>& b) noexcept
    -> enable_if_t<
         !is_same_v<
           executor_impl::property_list<OtherSupportableProperties...>,
           executor_impl::property_list<SupportableProperties...>
         >,
         bool
       >
  {
    return !(a == b);
  }

  friend bool operator==(const executor& e, nullptr_t) noexcept
  {
    return !e;
  }

  friend bool operator==(nullptr_t, const executor& e) noexcept
  {
    return !e;
  }

  friend bool operator!=(const executor& a, const executor& b) noexcept
  {
    return !(a == b);
  }

  friend bool operator!=(const executor& e, nullptr_t) noexcept
  {
    return !!e;
  }

  friend bool operator!=(nullptr_t, const executor& e) noexcept
  {
    return !!e;
  }

private:
  template<class, class, class...> friend class executor;
  executor(unique_ptr<executor_impl::impl_base<T, E>>&& i) noexcept : impl_(std::move(i)) {}

  executor_impl::impl_base<T, E> const* get_impl() const { return impl_.get(); }

  unique_ptr<executor_impl::impl_base<T, E>> impl_;

  // Take advantage of friendship to act as an attorney for the operator== implementations
  template <class... OtherSupportableProperties>
  static auto const* get_other_impl(executor<T, E, OtherSupportableProperties...> const& other) {
    return other.get_impl();
  }
};

// executor specialized algorithms:

template<class... SupportableProperties>
inline void swap(executor<SupportableProperties...>& a, executor<SupportableProperties...>& b) noexcept
{
  a.swap(b);
}

} // namespace execution
} // inline namespace executors_v1
} // namespace experimental
} // namespace std

#endif // STD_EXPERIMENTAL_BITS_EXECUTOR_H
