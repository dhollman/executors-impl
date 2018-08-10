#ifndef STD_EXPERIMENTAL_BITS_EXECUTOR_FACTORY_H
#define STD_EXPERIMENTAL_BITS_EXECUTOR_FACTORY_H

#include <experimental/execution>

#include <exception> // exception_ptr
#include <memory> // unique_ptr

namespace std {
namespace experimental {
inline namespace executors_v1 {
namespace execution {

namespace executor_factory_impl {

template <class T=void, class E=exception_ptr>
struct polymorphic_executor_factory_impl_base {
  virtual void* make_value_executor_impl(T&&) =0;
  virtual void* make_error_executor_impl(E&&) =0;
};

template <class E>
struct polymorphic_executor_factory_impl_base<void, E> {
  virtual void* make_value_executor_impl() =0;
  virtual void* make_error_executor_impl(E&&) =0;
};

template <class ConcreteFactory, class T, class E, class... SupportableProperties>
struct polymorphic_executor_factory_impl_common
  : polymorphic_executor_factory_impl_base<T, E>
{
  ConcreteFactory f_;

  explicit polymorphic_executor_factory_impl_common(ConcreteFactory&& f) : f_(std::move(f_)) { }

  void* make_error_executor_impl(E&& e) override {
    return new executor<T, E, SupportableProperties...>(
      f_.make_error_executor(std::move(e))
    );
  }
};

template <class ConcreteFactory, class T, class E, class... SupportableProperties>
struct polymorphic_executor_factory_impl
  : polymorphic_executor_factory_impl_common<ConcreteFactory, T, E, SupportableProperties...>
{

  void* make_value_executor_impl(T&& val) override {
    return new executor<T, E, SupportableProperties...>(
      this->f_.make_value_executor(std::move(val))
    );
  }

};

template <class ConcreteFactory, class E, class... SupportableProperties>
struct polymorphic_executor_factory_impl<ConcreteFactory, void, E, SupportableProperties...>
  : polymorphic_executor_factory_impl_common<ConcreteFactory, void, E, SupportableProperties...>
{

  using base_t = polymorphic_executor_factory_impl_common<ConcreteFactory, void, E, SupportableProperties...>;
  using base_t::base_t;

  void* make_value_executor_impl() override {
    return new executor<void, E, SupportableProperties...>(
      this->f_.make_value_executor()
    );
  }

};

template <class T, class E, class... SupportableProperties>
struct polymorphic_executor_factory_common {

  // TODO constrain to factory concept
  template <class ExecutorFactory,
    class=enable_if_t<not is_base_of_v<polymorphic_executor_factory_common, decay_t<ExecutorFactory>>>
  >
  explicit polymorphic_executor_factory_common(
    ExecutorFactory&& ef
  ) : impl_(
        make_unique<
          polymorphic_executor_factory_impl<ExecutorFactory, T, E, SupportableProperties...>
        >(forward<ExecutorFactory>(ef))
      )
  { }

  auto make_error_executor(E err) {
    return *static_cast<executor<T, E, SupportableProperties...>*>(
      impl_->make_error_executor_impl(std::move(err))
    );
  }

  std::unique_ptr<polymorphic_executor_factory_impl_base<T, E>> impl_;

};

template <class T, class E, class... SupportableProperties>
class polymorphic_executor_factory : private polymorphic_executor_factory_common<T, E, SupportableProperties...>
{
    using base_t = polymorphic_executor_factory_common<T, E, SupportableProperties...>;

  public:

    using base_t::base_t;

    auto make_executor(T val) {
      return *static_cast<executor<T, E, SupportableProperties...>*>(
        this->impl_->make_value_executor_impl(std::move(val))
      );
    }

};

template <class E, class... SupportableProperties>
class polymorphic_executor_factory<void, E, SupportableProperties...>
  : private polymorphic_executor_factory_common<void, E, SupportableProperties...>
{
    using base_t = polymorphic_executor_factory_common<void, E, SupportableProperties...>;

  public:

    using base_t::base_t;

    auto make_executor() {
      return *static_cast<executor<void, E, SupportableProperties...>*>(
        this->impl_->make_value_executor_impl()
      );
    }

};


template<class Derived>
struct property_base
{
  static constexpr bool is_requirable = false;
  static constexpr bool is_preferable = false;


  template<class Executor, class Type = decltype(Executor::query(*static_cast<Derived*>(0)))>
  static constexpr Type static_query_v = Executor::query(Derived());
};

} // namespace executor_factory_impl

template <class T, class E>
struct executor_factory_t : executor_factory_impl::property_base<executor_factory_t<T,E>>
{

  constexpr executor_factory_t() noexcept = default;

  template <class, class, class... SupportableProperties>
  using polymorphic_query_result_type =
    executor_factory_impl::polymorphic_executor_factory<T, E, SupportableProperties...>;
};

template <class T, class E>
constexpr executor_factory_t<T, E> executor_factory = { };

} // namespace execution
} // inline namespace executors_v1
} // namespace experimental
} // namespace std

#endif //STD_EXPERIMENTAL_BITS_EXECUTOR_FACTORY_H
