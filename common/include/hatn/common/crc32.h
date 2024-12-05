/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/crc32.h
  *
  *      Wrapper for CRC32.
  *
  */

/****************************************************************************/

#ifndef HATNCRC32_H
#define HATNCRC32_H

#include <boost/hana.hpp>

#include <hatn/thirdparty/crc32/crc32.hpp>

#include <hatn/common/format.h>

HATN_COMMON_NAMESPACE_BEGIN

template <typename ...ContainerT>
uint32_t Crc32(const ContainerT& ...data)
{
    auto each=[](uint32_t sum, auto&& buf)
    {
        return crc32<CRC32C_POLYNOMIAL>(sum, buf.begin(), buf.end());
    };
    return boost::hana::fold(boost::hana::make_tuple(data...),0xFFFFFFFF,each);
}

template <typename BufT, typename ...ContainerT>
void Crc32HexBuf(BufT& buf, const ContainerT& ...data)
{
    fmt::format_to(std::back_inserter(buf),"{:08x}",Crc32(data...));
}

template <typename ...ContainerT>
std::string Crc32Hex(const ContainerT& ...data)
{
    std::array<char,8> buf;
    fmt::format_to_n(buf.data(),8,"{:08x}",Crc32(data...));
    return fmtBufToString(buf);
}

constexpr const size_t Crc32HexLength=8;

HATN_COMMON_NAMESPACE_END

#endif // HATNCRC32_H
