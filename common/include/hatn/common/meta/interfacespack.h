/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/meta/interfacespack.h
  *
  *  COntains interfaces pack.
  *
  */

/****************************************************************************/

#ifndef HATNINTERFACESPACK_H
#define HATNINTERFACESPACK_H

#include <type_traits>
#include <tuple>

#include <hatn/common/common.h>

HATN_COMMON_NAMESPACE_BEGIN

/****************************** Extendable template class base to construct packs of interfaces ************************************/

//! Helper to get tuple's element by type name
template<int Index, class Search, class First, class... Types>
struct getT_internal
{
    typedef typename getT_internal<Index + 1, Search, Types...>::type type;
    static constexpr int index = Index;
};

//! Helper to get tuple's element by type name
template<int Index, class Search, class... Types>
struct getT_internal<Index, Search, Search, Types...>
{
    typedef getT_internal type;
    static constexpr int index = Index;
};

//! Get reference to tuple's element by type name
template<class T, class... Types>
constexpr T& getT(std::tuple<Types...>& tuple) noexcept
{
    return std::get<getT_internal<0,T,Types...>::type::index>(tuple);
}

//! Get const reference to tuple's element by type name
template<class T, class... Types>
constexpr const T& getT(const std::tuple<Types...>& tuple) noexcept
{
    return std::get<getT_internal<0,T,Types...>::type::index>(tuple);
}

//! Interfaces pack
template <typename ...Interfaces> class VInterfacesPack
{
public:

    VInterfacesPack(Interfaces&&... vals):m_interfaces(std::forward<Interfaces>(vals)...)
    {}

    VInterfacesPack()=default;
    ~VInterfacesPack()=default;
    VInterfacesPack(const VInterfacesPack&)=default;
    VInterfacesPack(VInterfacesPack&&)=default;
    VInterfacesPack& operator=(const VInterfacesPack&)=default;
    VInterfacesPack& operator=(VInterfacesPack&&)=default;

    //! Type by index
    template <int Index> using type=typename std::tuple_element<Index, std::tuple<Interfaces...>>::type;

    //! Get position of interface in the tuple
    template <typename T> constexpr inline static int getInterfacePos() noexcept
    {
        return getT_internal<0,T,Interfaces...>::type::index;
    }

    //! Get interface by type
    template <typename T> constexpr T& getInterface() noexcept
    {
        return getT<T>(m_interfaces);
    }

    //! Get interface by type
    template <typename T> constexpr const T& getInterface() const noexcept
    {
        return getT<T>(m_interfaces);
    }

    //! Get interface by index
    template <int Index>
    constexpr typename std::tuple_element<Index, std::tuple<Interfaces...>>::type&
    getInterface() noexcept
    {
        return std::get<Index>(m_interfaces);
    }

    //! Get const interface by index
    template <int Index>
    constexpr const typename std::tuple_element<Index, std::tuple<Interfaces...>>::type&
    getInterface() const noexcept
    {
        return std::get<Index>(m_interfaces);
    }

    constexpr static const int InterfaceCount=sizeof...(Interfaces);
    constexpr static const int MaxInterfaceIndex=InterfaceCount-1;

protected:

    std::tuple<Interfaces...> m_interfaces;
};

//! Base class for classes with interfaces
class InterfacesBase
{
    //! Get specific interface
    template <typename Interface,typename ...Interfaces>
    constexpr static Interface& interface(common::VInterfacesPack<Interfaces...>* interfaces) noexcept
    {
        return interfaces->template getInterface<Interface>();
    }

    //! Get specific interface
    template <typename Interface,typename ...Interfaces>
    constexpr static const Interface& interface(const common::VInterfacesPack<Interfaces...>* interfaces) noexcept
    {
        return interfaces->template getInterface<Interface>();
    }
};

HATN_COMMON_NAMESPACE_END

#endif // HATNINTERFACESPACK_H
