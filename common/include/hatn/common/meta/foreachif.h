/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/meta/foreachif.h
  *
  * Defines constexpr foreac_if.
  *
  */

/****************************************************************************/

#ifndef HATNFOREACHIF_H
#define HATNFOREACHIF_H

#include <boost/hana.hpp>

#include <hatn/common/common.h>

HATN_COMMON_NAMESPACE_BEGIN

//---------------------------------------------------------------

namespace detail {

/**
 * @brief Implementations of foreach_if().
 */
template <typename IndexT>
struct foreach_if_t
{
    template <typename T, typename HandlerT, typename PredicateT>
    static auto each(T&& obj, const PredicateT& pred, const HandlerT& fn)
    {
        auto index=IndexT{};
        auto&& val=boost::hana::at(obj,index);
        auto res=fn(val,index);

        return hana::eval_if(
            pred(res),
            [&](auto _)
            {
                return _(res);
            },
            [&](auto _)
            {
                auto next_index=boost::hana::plus(_(index),boost::hana::size_c<1>);
                return boost::hana::eval_if(
                    boost::hana::equal(next_index,boost::hana::size(_(obj))),
                    [&](auto&& _)
                    {
                        return _(res);
                    },
                    [&](auto&& _)
                    {
                        auto index=_(next_index);
                        return foreach_if_t<decltype(index)>::each(_(obj),_(pred),_(fn));
                    }
                );
            }
        );
    }
};

} // namespace detail

/**
 * @brief Implementer of foreach_if().
 */
struct foreach_if_impl
{
    /**
     * @brief Invoke a handler on each element of an object till predicate is satisfied.
     * @param obj Object.
     * @param pred Predicate.
     * @param handler Handler to invoke.
     * @return Accumulated result of handler invocations or false if object is not a heterogeneous container.
     */
    template <typename T, typename PredicateT, typename HandlerT>
    auto operator () (T&& obj, const PredicateT& pred, const HandlerT& handler) const
    {
        return detail::foreach_if_t<boost::hana::size_t<0>>::each(obj,pred,handler);
    }
};
/**
 * @brief Invoke a handler on each element of a foldable container while predicate is satisfied.
 *
 *  Predicate must return a type of hana Logical concept, e.g. hana::bool_ or std::integral_constant<bool>.
 *
 *  Handler signature must be: "RetT (auto&& element, auto&& element_index)".
 */
constexpr foreach_if_impl foreach_if{};

HATN_COMMON_NAMESPACE_END

#endif // HATNFOREACHIF_H
