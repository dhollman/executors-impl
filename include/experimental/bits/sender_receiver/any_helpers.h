
#pragma once

#include <memory> // std::unique_ptr

namespace std {
namespace experimental {
inline namespace executors_v1 {
namespace execution {

template <class Derived>
struct __cloneable
{
    virtual ~__cloneable() {}
    virtual unique_ptr<Derived> clone() const = 0;
};

template <class Interface>
  requires is_abstract_v<Interface> && requires (Interface const* p) {
      { p->clone() } -> unique_ptr<Interface>;
  }
class __pimpl_ptr
{
    unique_ptr<Interface> ptr_;
public:
    __pimpl_ptr() = default;
    __pimpl_ptr(unique_ptr<Interface> ptr) : ptr_(std::move(ptr)) {}
    __pimpl_ptr(__pimpl_ptr&&) = default;
    __pimpl_ptr(__pimpl_ptr const& that)
      : ptr_(that.ptr_ ? that.ptr_->clone() : unique_ptr<Interface>{}) {}
    __pimpl_ptr& operator=(__pimpl_ptr&&) = default;
    __pimpl_ptr& operator=(__pimpl_ptr const& that)
    {
        if (&that != this)
            ptr_ = (that.ptr_ ? that.ptr_->clone() : unique_ptr<Interface>{});
        return *this;
    }
    Interface* operator->() noexcept { return ptr_.get(); }
    Interface const* operator->() const noexcept { return ptr_.get(); }
    Interface& operator*() noexcept { return *ptr_; }
    Interface const& operator*() const noexcept { return *ptr_; }
    operator bool() const noexcept { return !!ptr_; }
};

template <class...>
struct __typelist;

} // end namespace execution
} // end inline namespace executors_v1
} // end namespace experimental
} // end namespace std