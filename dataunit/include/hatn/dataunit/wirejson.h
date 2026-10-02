/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file dataunit/wirejson.h
  *
  *     Schemaless transcoding of a wire-format (protobuf-compatible) blob to JSON.
  */

/****************************************************************************/

#ifndef HATNDATAUNITWIREJSON_H
#define HATNDATAUNITWIREJSON_H

#include <string>

#include <hatn/common/error.h>
#include <hatn/common/result.h>

#include <hatn/dataunit/dataunit.h>

HATN_DATAUNIT_NAMESPACE_BEGIN

/**
 * @brief Transcode a serialized unit to JSON without knowing its schema.
 *
 * The result is a JSON object whose keys are the decimal field numbers found on the wire; a field
 * number that occurs more than once becomes an array, in wire order. Values:
 *  - VarInt, Fixed32, Fixed64: an unsigned number (signedness and zigzag encoding cannot be told
 *    from the wire, so negative values come out as large unsigned numbers);
 *  - length-delimited: a nested object when the content parses completely as a message (and
 *    @a maxDepth is not exceeded), otherwise a string when it is valid UTF-8 text, otherwise the
 *    Base64 of the bytes.
 *
 * A length-delimited field cannot be told apart from the wire alone - bytes, text, a nested message
 * and a packed repeated scalar all look the same - so the result is a best effort meant for
 * inspection and fallback display, not a substitute for the real schema. A packed repeated scalar
 * comes out as a string or bytes.
 *
 * @param data Blob to transcode.
 * @param size Its size.
 * @param maxDepth Deepest nesting of messages that is tried; deeper length-delimited fields are
 *        rendered as text or bytes.
 * @return The JSON, or an error when the blob is not a well-formed wire-format message at its top
 *         level (an unknown wire type, a truncated value, a length past the end).
 */
HATN_DATAUNIT_EXPORT common::Result<std::string> wireToJson(const char* data, size_t size, size_t maxDepth=8);

HATN_DATAUNIT_NAMESPACE_END

#endif // HATNDATAUNITWIREJSON_H
