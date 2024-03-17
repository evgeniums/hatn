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
#endif

#if __cplusplus < 201703L || (defined (IOS_SDK_VERSION_X10) && IOS_SDK_VERSION_X10<120)
    #include <boost/variant.hpp>
    #include <boost/optional.hpp>
#else
    #include <variant>
    #include <optional>
#endif

#include <fmt/core.h>

#include <hatn/common/common.h>

HATN_COMMON_NAMESPACE_BEGIN
namespace lib
{

#if __cplusplus < 201703L
    using string_view=boost::string_view;
#else
    using string_view=std::string_view;
#endif
template <typename T> string_view toStringView(const T& buf) noexcept
{
    return string_view(buf.data(),buf.size());
}

#if __cplusplus < 201703L || (defined (IOS_SDK_VERSION_X10) && IOS_SDK_VERSION_X10<120)
    template <typename ...Types> using variant=boost::variant<Types...>;
    template <typename T,typename ...Types> constexpr T& variantGet(variant<Types...>& var) noexcept
    {
        return boost::get<T>(var);
    }
    template <typename T,typename ...Types> constexpr const T& variantGet(const variant<Types...>& var) noexcept
    {
        return boost::get<T>(var);
    }
    template <typename ...Types> constexpr std::size_t variantIndex(const variant<Types...>& var) noexcept
    {
        return static_cast<std::size_t>(var.which());
    }
    template <typename T> using optional=boost::optional<T>;
#else
    template <typename ...Types> using variant=std::variant<Types...>;
    template <typename T,typename ...Types> constexpr T& variantGet(variant<Types...>& var) noexcept
    {
        return std::get<T>(var);
    }
    template <typename T,typename ...Types> constexpr const T& variantGet(const variant<Types...>& var) noexcept
    {
        return std::get<T>(var);
    }
    template <typename ...Types> constexpr std::size_t variantIndex(const variant<Types...>& var) noexcept
    {
        return var.index();
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
#endif // HATNSTDWRAPPERS_H
