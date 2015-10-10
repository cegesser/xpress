#pragma once

#include <boost/variant.hpp>

namespace xpress {
namespace detail {

/**
 * Struct to group a set of lambdas.
 * Inherits from a list of lambdas.
 */
template<class Lambda, class... Lambdas>
struct multilambda : Lambda, multilambda<Lambdas...>
{
    using Lambda::operator();
    using multilambda<Lambdas...>::operator();

    inline multilambda(Lambda&& lambda, Lambdas&&... lambdas)
      : Lambda(std::move(lambda))
      , multilambda<Lambdas...>(std::move(lambdas)...)
    {}
};

/**
 * Single lambda case.
 */
template<class Lambda>
struct multilambda<Lambda> : Lambda
{
    using Lambda::operator();

    inline multilambda(Lambda&& f)
      : Lambda(std::move(f))
    {}
};

/**
 * This is the variant visitor implementation, it calls the multilambda
 */
template <typename ReturnType, typename... Lambdas>
struct lambda_visitor : public boost::static_visitor<ReturnType> {
    multilambda<Lambdas...> lambdas;

    template<typename... L>
    inline lambda_visitor(L &&...lambdas) : lambdas(  std::forward<L>(lambdas)...) {}

    template<typename V>
    inline ReturnType operator()(V &&v) const { return lambdas(std::forward<V>(v)); }
};

/**
 * Typedef to get the first type of a variant
 */
template<typename Variant>
using VariantFirstType = typename boost::mpl::front<typename Variant::types>;

/**
 * Template to get the result type of a lambda, being passed the first type of a variant
 */
template<typename Variant, typename... Lambdas>
struct ResultOf
{
    using variant_type = typename detail::VariantFirstType<Variant>::type;
    using type = typename std::result_of<multilambda<Lambdas...>( variant_type )>::type;
};

/**
 * Helper typedef to simplify the previous one
 */
template<typename Variant, typename... Lambdas>
using LambdaResult = typename  detail::ResultOf<typename std::remove_reference<Variant>::type, Lambdas...>::type;

} // namespace detail

/**
 * Matches a variant against a set of lambdas.
 * All variant possibilities must be covered by the lambdas, and all of them must have the same return type.
 */
template <typename Variant, typename... Lambdas>
inline detail::LambdaResult<Variant, Lambdas...> type_switch(Variant &&variant, Lambdas &&... lambdas) {
    using Visitor = detail::lambda_visitor< detail::LambdaResult<Variant, Lambdas...>, Lambdas... >;
    Visitor visitor( std::forward<Lambdas>(lambdas)... );
    return std::forward<Variant>(variant).apply_visitor(visitor);
}

namespace detail {

/**
 * Helper visitor to check a variant type
 */
template<typename T>
struct IsTypeVisitor : public boost::static_visitor<bool> {
    inline bool operator()(const T &) const { return true; }

    template<typename U>
    inline bool operator()(const U &) const { return false; }
};

}

/**
 * Returns true if the given variant is storing an object of the given type
 */
template<typename T, typename Variant>
inline bool type_matches(Variant &&variant) {
    detail::IsTypeVisitor<T> visitor;
    return std::forward<Variant>(variant).apply_visitor(visitor);
}

} // namespace gesser
