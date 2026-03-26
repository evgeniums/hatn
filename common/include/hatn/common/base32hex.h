/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/base32hex.h
  *
  *      Wrapper for Base32Hex.
  *
  */

/****************************************************************************/

#ifndef HATNBASE32HEX_H
#define HATNBASE32HEX_H

#include <cppcodec/base32_hex.hpp>

#include <hatn/common/common.h>

HATN_COMMON_NAMESPACE_BEGIN

namespace detail {

class base32_hex_unpadded_traits : public cppcodec::detail::base32_hex
{
    public:

        template <typename Codec> using codec_impl = cppcodec::detail::stream_codec<Codec,base32_hex_unpadded_traits>;

        static CPPCODEC_ALWAYS_INLINE constexpr bool generates_padding() { return false; }
        static CPPCODEC_ALWAYS_INLINE constexpr bool requires_padding() { return false; }
};

using base32_hex_unpadded = cppcodec::detail::codec<cppcodec::detail::base32<base32_hex_unpadded_traits>>;

template <typename Codec>
class base32_hex_append : public Codec
{
    public:

        template <typename Result>
        static size_t encodeAppend(Result& result, const char* inputData, size_t inputSize)
        {
            auto encodedSize = Codec::encoded_size(inputSize);

            size_t offset = result.size();
            result.resize(offset + encodedSize);

            return Codec::encode(&result[offset], encodedSize, inputData, inputSize);
        }

        template <typename Result, typename Input>
        static size_t decodeAppend(Result& result, const char* inputData, size_t inputSize)
        {
            size_t maxDecodedSize = Codec::decoded_max_size(inputSize);

            size_t bufOffset = result.size();
            result.resize(bufOffset + maxDecodedSize);

            size_t actualDecodedSize = Codec::decode(
                result.data()+bufOffset,
                maxDecodedSize,
                inputData,
                inputSize
            );

            result.resize(result + actualDecodedSize);
            return actualDecodedSize;
        }

        template <typename Result, typename Input>
        static size_t encodeAppend(Result& result, const Input& input)
        {
            return encodeAppend(result,input.data(),input.size());
        }

        template <typename Result, typename Input>
        static size_t decodeAppend(Result& result, const Input& input)
        {
            return decodeAppend(result,input.data(),input.size());
        }
};

}

using Base32Hex = detail::base32_hex_append<cppcodec::base32_hex>;
using Base32HexUnpadded = detail::base32_hex_append<detail::base32_hex_unpadded>;

HATN_COMMON_NAMESPACE_END

#endif // HATNBASE32HEX_H
