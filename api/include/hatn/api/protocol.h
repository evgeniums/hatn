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

#include <array>

#include <boost/endian/conversion.hpp>

#include <hatn/common/errorcodes.h>

#include <hatn/api/api.h>
#include <hatn/api/apiconstants.h>

#define HATN_API_RESPONSE_STATUS(Do) \
    Do(ApiResponseStatus,Success,_TR("success","api_protocol",translator)) \
    Do(ApiResponseStatus,AuthError,_TR("authentification error","api_protocol",translator)) \
    Do(ApiResponseStatus,InternalServerError,_TR("internal server error","api_protocol",translator)) \
    Do(ApiResponseStatus,FormatError,_TR("invalid data format","api_protocol",translator)) \
    Do(ApiResponseStatus,ValidationError,_TR("data validation failed","api_protocol",translator)) \
    Do(ApiResponseStatus,RoutingError,_TR("cannot route request","api_protocol",translator)) \
    Do(ApiResponseStatus,RequestTooBig,_TR("size of request is too big","api_protocol",translator)) \
    Do(ApiResponseStatus,UnknownMethod,_TR("unknown method of request","api_protocol",translator)) \
    Do(ApiResponseStatus,InvalidMessageType,_TR("invalid type of message in request","api_protocol",translator)) \
    Do(ApiResponseStatus,MessageMissing,_TR("missing message in request","api_protocol",translator)) \
    Do(ApiResponseStatus,ExecFailed,_TR("failed to execute request","api_protocol",translator)) \
    Do(ApiResponseStatus,RetryLater,_TR("server not ready to process the request, please retry later","api_protocol",translator)) \
    Do(ApiResponseStatus,ForeignServerFailed,_TR("could not forward request to foreign server, please retry later","api_protocol",translator))

HATN_API_NAMESPACE_BEGIN

namespace protocol
{

constexpr const size_t HEADER_LENGTH=4;
constexpr const size_t DEFAULT_MAX_MESSAGE_SIZE=0x1000000;

constexpr const size_t ServiceNameLengthMax=16;
constexpr const size_t MethodNameLengthMax=32;
constexpr const size_t UnitNameLengthMax=32;
constexpr const size_t AuthProtocolNameLengthMax=8;
constexpr const size_t ResponseMsgTypeLengthMax=32;
constexpr const size_t ResponseFamilyNameLengthMax=32;
constexpr const size_t ResponseStatusLengthMax=32;
constexpr const size_t TenancyIdLengthMax=32;
constexpr const size_t LocaleNameLengthMax=16;
constexpr const size_t IpAddressLength=32;

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
    HATN_API_RESPONSE_STATUS(HATN_ERROR_CODE)
};

constexpr const char* const ResponseStatusStrings[] = {
    HATN_API_RESPONSE_STATUS(HATN_PLAIN_ERROR_STR)
};

inline const char* responseStatusString(ResponseStatus status)
{
    return errorString(status,ResponseStatusStrings);
}

} // namespace protocol

using ApiResponseStatus=protocol::ResponseStatus;

HATN_API_NAMESPACE_END

#endif // HATNAPIPROTOCOL_H
