/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/containerutils.h
  *
  *      Utils for containers.
  *
  */

/****************************************************************************/

#ifndef HATNCONTAINERUTILS_H
#define HATNCONTAINERUTILS_H

#include <boost/algorithm/hex.hpp>

#include <hatn/thirdparty/base64/base64.h>
#include <hatn/common/types.h>
#include <hatn/common/bytearray.h>

HATN_COMMON_NAMESPACE_BEGIN

//! Utils for containers
struct ContainerUtils final
{
    ContainerUtils()=delete;
    ~ContainerUtils()=delete;
    ContainerUtils(const ContainerUtils&)=delete;
    ContainerUtils(ContainerUtils&&) =delete;
    ContainerUtils& operator=(const ContainerUtils&)=delete;
    ContainerUtils& operator=(ContainerUtils&&) =delete;

    /**
     * @brief Convert hex representation to raw data
     * @param hexContainer Container with hex-encoded data
     * @param rawContainer Traget container with raw data
     * @throws boost::exception
     */
    template <typename ContainerHexT, typename ContainerRawT>
    static void hexToRaw(
            const ContainerHexT& hexContainer,
            ContainerRawT& rawContainer
        )
    {
        boost::algorithm::unhex(hexContainer.begin(),hexContainer.end(),std::back_insert_iterator<ContainerRawT>(rawContainer));
    }

    /**
     * @brief Convert raw data to hex representation
     * @param rawContainer Container with raw data
     * @param hexContainer Target container with hex-encoded data
     * @param lowerCase Lower or upper case
     */
    template <typename ContainerHexT, typename ContainerRawT>
    static void rawToHex(
            const ContainerRawT& rawContainer,
            ContainerHexT& hexContainer,
            bool lowerCase=false
        )
    {
        if (lowerCase)
        {
            boost::algorithm::hex_lower(rawContainer.begin(),rawContainer.end(),std::back_insert_iterator<ContainerHexT>(hexContainer));
        }
        else
        {
            boost::algorithm::hex(rawContainer.begin(),rawContainer.end(),std::back_insert_iterator<ContainerHexT>(hexContainer));
        }
    }

    /**
     * @brief Convert base64 data representation to raw data
     * @param b64Container Container with base64-encoded data
     * @param rawContainer Tartget raw container
     */
    template <typename ContainerB64T, typename ContainerRawT>
    static void base64ToRaw(
            const ContainerB64T& b64Container,
            ContainerRawT& rawContainer
        )
    {
        Base64::from(b64Container.data(),b64Container.size(),rawContainer);
    }

    /**
     * @brief Convert raw data to base64-encoded representation
     * @param rawContainer Raw data
     * @param b64Container Target container with base64-encoded data
     * @param append If true then append data to raw container, if false then overwrite raw container
     */
    template <typename ContainerB64T, typename ContainerRawT>
    static void rawToBase64(
            const ContainerRawT& rawContainer,
            ContainerB64T& b64Container,
            bool append=false
        )
    {
        Base64::to(rawContainer.data(),rawContainer.size(),b64Container,append);
    }
};

template <typename T> class HasDataMethod
{
    typedef char one;
    typedef long two;

    template <typename C> static one test(decltype(std::declval<const C>().data())) ;
    template <typename C> static two test(...);

  public:

    enum { value = sizeof(test<T>(0)) == sizeof(char) };
};
template <typename T> class HasSizeMethod
{
    typedef char one;
    typedef long two;

    template <typename C> static one test(decltype(&C::size)) ;
    template <typename C> static two test(...);

  public:

    enum { value = sizeof(test<T>(0)) == sizeof(char) };
};
template <typename T> struct IsContainer
{
    constexpr static const bool value = HasDataMethod<T>::value && HasSizeMethod<T>::value;
};

//---------------------------------------------------------------
HATN_COMMON_NAMESPACE_END
#endif // HATNCONTAINERUTILS_H
