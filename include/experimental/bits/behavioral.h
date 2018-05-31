#ifndef STD_EXPERIMENTAL_BITS_BEHAVIORAL_H
#define STD_EXPERIMENTAL_BITS_BEHAVIORAL_H

#include <experimental/execution>

#include <experimental/type_traits>

#include <type_traits>
#include <cstdint>

namespace std {
namespace experimental {
inline namespace executors_v1 {
namespace execution {
namespace behavioral_properties_impl {


template <typename Derived>
struct property_base {

  template <typename Executor,
    typename StaticQueryType=decltype(Executor::query(std::declval<Derived&>()))
  >
  static constexpr StaticQueryType static_query_v = Executor::query(Derived());

};

//==============================================================================

template <typename Derived, typename S>
struct nested_behavioral_property_base : property_base<Derived> {
  public:
    using polymorphic_query_result_type = S;

    static constexpr bool is_requirable = true;

    template <typename DD=Derived, typename ShouldBeS=decltype(S(DD()))>
    static constexpr ShouldBeS value() { return S(DD()); }
};

//==============================================================================

template <typename N1, typename S, typename... NN>
struct first_nested_behavioral_property_base
{
  private:

    template <typename Executor, typename Nested1>
    using _has_static_query_archetype = decltype(
      Executor::query(Nested1{})
    );

    template <typename Executor, typename Nested1>
    static constexpr bool has_static_query_v = std::experimental::is_detected_v<
      _has_static_query_archetype, Executor, Nested1
    >;

    template <typename Executor, typename Nested>
    using _has_dynamic_query_archetype = decltype(std::declval<Executor>().query(Nested()));

    template <typename Executor>
    static constexpr bool has_dynamic_query_v = std::experimental::is_detected_v<
      _has_dynamic_query_archetype, Executor, N1
    >;

    /**
     *  The value of the expression S::N1::static_query_v<Executor> is
     *
     *    - Executor::query(S::N1()), if that expression is a well-formed expression;
     *    - ...
     *
     */
    template <typename Executor,
      typename Nested1=N1,
      typename=std::enable_if_t<
        has_static_query_v<Executor, Nested1>
      >
    >
    static constexpr auto _static_query() {
      return Executor::query(Nested1{});
    };

    /**
     *
     *  The value of the expression S::N1::static_query_v<Executor> is
     *
     *    - ...
     *    - ill-formed if declval<Executor>().query(S::N1()) is well-formed;
     *    - ill-formed if can_query_v<Executor,S::Ni> is true for any 1 < i <= N;
     *    - otherwise S::N1().
     *
     */
    template <typename Executor, typename Nested1=N1>
    static constexpr auto _static_query(
      std::enable_if_t<
        !has_static_query_v<Executor, Nested1>
          && !has_dynamic_query_v<Executor>
          && !std::conjunction_v<std::bool_constant<std::experimental::execution::can_query_v<Executor, NN>>...>,
        int
      > = 0
    ) {
      return Nested1{};
    };


  public:
    using polymorphic_query_result_type = S;

    static constexpr bool is_requirable = true;

    template <typename Nested1=N1, typename ShouldBeS=decltype(S(Nested1()))>
    static constexpr ShouldBeS value() { return ShouldBeS(Nested1()); }

    template <typename Executor>
    static constexpr auto static_query_v =
      first_nested_behavioral_property_base::template _static_query<Executor>();

};

//==============================================================================

template <typename ID, ID I, typename S, typename... NN>
struct _behavioral_property_nested_helper;

template <typename ID, ID I, typename S, typename NK, typename... NN>
struct _behavioral_property_nested_helper<ID, I, S, NK, NN...>
  : _behavioral_property_nested_helper<ID, I+1, S, NN...>
{
  private:

    template <typename Executor, typename SS>
    using _has_dynamic_query_archetype = decltype(std::declval<Executor>().query(SS()));

    template <typename Executor>
    static constexpr bool has_dynamic_query_v = std::experimental::is_detected_v<
      _has_dynamic_query_archetype, Executor, S
    >;

    template <typename Executor, typename NestedK>
    using _has_static_query_archetype = decltype(
      NestedK::template static_query_v<Executor>
    );

    template <typename Executor, typename NestedK>
    static constexpr bool has_static_query_v = std::experimental::is_detected_v<
      _has_static_query_archetype, Executor, NestedK
    >;

    using base_t = _behavioral_property_nested_helper<ID, I+1, S, NN...>;

  protected:

    inline constexpr explicit
    _behavioral_property_nested_helper(ID id)
      : base_t(id)
    { }

    /**
     * The value of the expression S::static_query_v<Executor> is
     *
     *   - ...
     *   - otherwise, ill-formed if declval<Executor>().query(S()) is well-formed;
     *   - otherwise, S::Ni::static_query_v<Executor> for the least i <= N for
     *     which this expression is a well-formed
     *   - ...
     *
     */
    template <typename Executor,
      typename NestedK=NK,
      typename=std::enable_if_t<
        !has_dynamic_query_v<Executor>
          and has_static_query_v<Executor, NestedK>
      >
    >
    static constexpr auto _static_query() {
      return NestedK::template static_query_v<Executor>;
    }

    template <typename Executor, typename NestedK=NK>
    static constexpr auto _static_query(
      std::enable_if_t<
        !has_dynamic_query_v<Executor>
          and !has_static_query_v<Executor, NestedK>
          and sizeof...(NN) != 0,
        int
      > = 0
    ) {
      return base_t::template _static_query<Executor>();
    }

  public:

    using base_t::base_t;

    template <typename AlwaysNK=NK, ID=I>
    inline constexpr
    _behavioral_property_nested_helper(
      // use decay_t to block deduction:
      std::decay_t<AlwaysNK>,
      // use a second parameter to make this distinct from base class candidates
      std::integral_constant<ID, I> = { }
    ) : base_t(I)
    { }



};

template <typename ID, ID I, typename S>
struct _behavioral_property_nested_helper<ID, I, S> {
  protected:

    ID mode_id_;

    inline constexpr explicit
    _behavioral_property_nested_helper(ID id) : mode_id_(id)
    { }

    static constexpr void _static_query(int, int, int) { }

    /**
     * The value of the expression S::static_query_v<Executor> is
     *
     *   - ...
     *   - otherwise ill-formed
     *
     */

};


template <typename S, typename N1, typename... NN>
struct behavioral_property_base
  : _behavioral_property_nested_helper<uint8_t, 1, S, N1, NN...>
{
  private:

    using base_t = _behavioral_property_nested_helper<uint8_t, 1, S, N1, NN...>;

    /**
     * The value of the expression S::static_query_v<Executor> is
     *
     *   - Executor::query(S()), if that expression is a well-formed constant expression;
     *   - ...
     *
     */
    template <typename Executor>
    using _static_query_t = decltype(Executor::query(std::declval<S&>()));

    template <typename Executor,
      typename=std::enable_if_t<
        std::experimental::is_detected_v<_static_query_t, Executor>
      >
    >
    static constexpr auto _static_query() {
      return Executor::query(S());
    };

    template <typename Executor>
    static constexpr auto _static_query(
      std::enable_if_t<
        !std::experimental::is_detected_v<_static_query_t, Executor>,
        int
      > = { }
    ) {
      return base_t::template _static_query<Executor>();
    };


  public:

    using base_t::base_t;

    inline constexpr
    behavioral_property_base()
      : base_t(0 /* just the bare property */) { }

    template <typename Executor>
    static constexpr auto static_query_v =
      behavioral_property_base::template _static_query<Executor>();

};

} // end namespace behavioral_properties_impl
} // end namespace execution
} // end namespace executors_v1
} // end namespace experimental
} // end namespace std

#endif //STD_EXPERIMENTAL_BITS_BEHAVIORAL_H
