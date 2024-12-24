/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/stdwrappers.h
  *
  *      Wrappers of types, methods and atributes that can be present or missing in std lib.
  *
  */

/****************************************************************************/

#ifndef HATNSTDWRAPPERS_H
#define HATNSTDWRAPPERS_H

#include <memory>

#if __cplusplus < 201703L
    #include <boost/utility/string_view.hpp>
    #include <boost/thread/shared_mutex.hpp>
    #include <boost/thread/locks.hpp>
    #include <boost/thread/lock_types.hpp>
    #define HATN_MAYBE_CONSTEXPR
#else
    #define HATN_MAYBE_CONSTEXPR constexpr
    #include <shared_mutex>
#endif

#if __cplusplus < 201703L
    #include <boost/variant2.hpp>
    #include <boost/optional.hpp>
#else
    #include <variant>
    #include <optional>
    #include <mutex>
#endif

#include <fmt/core.h>

#include <hatn/common/common.h>

HATN_COMMON_NAMESPACE_BEGIN
namespace lib
{

#if __cplusplus < 201703L
    using string_view=boost::string_view;
    using shared_mutex=boost::shared_mutex;
    template <typename T> using shared_lock=boost::shared_lock<T>;
    template <typename T> using unique_lock=boost::unique_lock<T>;
#else
    using string_view=std::string_view;
    using shared_mutex=std::shared_mutex;
    template <typename T> using shared_lock=std::shared_lock<T>;
    template <typename T> using unique_lock=std::unique_lock<T>;
#endif

template <typename T> string_view toStringView(const T& buf) noexcept
{
    return string_view(buf.data(),buf.size());
}

#if __cplusplus < 201703L
template <typename ...Types> using variant=boost::variant2::variant<Types...>;
    template <typename T,typename ...Types>
    constexpr T& variantGet(variant<Types...>& var) noexcept
    {
        return boost::variant2::get<T>(var);
    }
    template <typename T,typename ...Types>
    constexpr const T& variantGet(const variant<Types...>& var) noexcept
    {
        return boost::variant2::get<T>(var);
    }
    template <typename ...Types>
    constexpr std::size_t variantIndex(const variant<Types...>& var) noexcept
    {
        return static_cast<std::size_t>(var.index());
    }
    template<typename Visitor, typename Variant>
    decltype(auto) variantVisit(Visitor&& vis, Variant&& var)
    {
        return boost::variant2::visit(std::forward<Visitor>(vis),std::forward<Variant>(var));
    }
    template <typename T> using optional=boost::optional<T>;
#else
    template <typename ...Types> using variant=std::variant<Types...>;
    template <typename T,typename ...Types>
    constexpr T& variantGet(variant<Types...>& var) noexcept
    {
        return std::get<T>(var);
    }
    template <typename T,typename ...Types>
    constexpr const T& variantGet(const variant<Types...>& var) noexcept
    {
        return std::get<T>(var);
    }
    template <typename ...Types>
    constexpr std::size_t variantIndex(const variant<Types...>& var) noexcept
    {
        return var.index();
    }
    template<typename Visitor, typename Variant>
    decltype(auto) variantVisit(Visitor&& vis, Variant&& var)
    {
        return std::visit(std::forward<Visitor>(vis),std::forward<Variant>(var));
    }
    template <typename T> using optional=std::optional<T>;
    #define HATN_VARIANT_CPP17
#endif

template <typename T> inline void destroyAt(T* obj) noexcept
{
#if __cplusplus < 201703L
    obj->~T();
#else
    std::destroy_at(obj);
#endif
}

}

#if __cplusplus >= 201703L
#define HATN_FALLTHROUGH [[fallthrough]];
#else
#define HATN_FALLTHROUGH
#endif

#if __cplusplus >= 201703L && !(defined (__GNUC__) && defined (_WIN32))
#define HATN_NODISCARD [[nodiscard]]
#else
#define HATN_NODISCARD
#endif

//---------------------------------------------------------------
HATN_COMMON_NAMESPACE_END

HATN_NAMESPACE_BEGIN

namespace lib=common::lib;

HATN_NAMESPACE_END
#endif // HATNSTDWRAPPERS_H
