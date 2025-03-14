/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file api/protocol.h
  *
  */

/****************************************************************************/

#ifndef HATNAPIPROTOCOL_H
#define HATNAPIPROTOCOL_H

#include <boost/endian/conversion.hpp>

#include <hatn/api/api.h>
#include <hatn/api/apiconstants.h>

HATN_API_NAMESPACE_BEGIN

namespace protocol
{

constexpr const size_t HEADER_LENGTH=4;
constexpr const size_t DEFAULT_MAX_MESSAGE_SIZE=0x1000000;

constexpr const size_t ServiceNameLengthMax=16;
constexpr const size_t MethodNameLengthMax=32;
constexpr const size_t UnitNameLengthMax=32;
constexpr const size_t AuthProtocolNameLengthMax=8;
constexpr const size_t ResponseCategoryNameLengthMax=32;
constexpr const size_t ResponseFamilyNameLengthMax=32;
constexpr const size_t ResponseStatusLengthMax=32;
constexpr const size_t TenancyIdLengthMax=32;

inline uint32_t bufToSize(const char* buf) noexcept
{
    uint32_t size=0;
    memcpy(&size,buf,HEADER_LENGTH);
    boost::endian::little_to_native_inplace(size);
    return size;
}

inline void sizeToBuf(uint32_t size, char* buf) noexcept
{
    uint32_t littleEndianSize=boost::endian::native_to_little(size);
    memcpy(buf,&littleEndianSize,HEADER_LENGTH);
}

class Header
{
    public:

        void setMessageSize(uint32_t size) noexcept
        {
            sizeToBuf(size,m_headerBuf.data());
        }

        uint32_t messageSize() const noexcept
        {
            return bufToSize(m_headerBuf.data());
        }

        char* data()
        {
            return m_headerBuf.data();
        }

        const char* data() const
        {
            return m_headerBuf.data();
        }

        size_t size() const noexcept
        {
            return m_headerBuf.size();
        }

    private:

        std::array<char,protocol::HEADER_LENGTH> m_headerBuf;
};

enum class ResponseStatus : int
{
    OK=0,
    AuthError=1,
    InternalServerError=2,
    RoutingError=3,
    ServiceError=4
};

}

HATN_API_NAMESPACE_END

#endif // HATNAPIPROTOCOL_H
