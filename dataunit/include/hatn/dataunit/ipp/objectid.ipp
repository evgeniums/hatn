/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file dataunit/ipp/objectid.ipp
  *
  * Contains definitions of ObjectId template methods.
  *
  */

/****************************************************************************/

#ifndef HATNOBJECTID_IPP
#define HATNOBJECTID_IPP

#include <hatn/dataunit/rapidjsonsaxhandlers.h>
#include <hatn/dataunit/fieldserialization.h>

#include <hatn/dataunit/objectid.h>

HATN_DATAUNIT_NAMESPACE_BEGIN

template <typename BufferT>
bool OidTraits::serialize(const ObjectId& val, BufferT& wired)
{
    auto buf=AsBytesSer::serializePrepare(wired,ObjectId::Length);
    val.serialize(buf);
    return true;
}

template <typename BufferT>
bool OidTraits::deserialize(ObjectId& val, BufferT& wired, const AllocatorFactory*)
{
    return HATN_DATAUNIT_NAMESPACE::AsBytesSer::deserialize(wired,
        [&val](const char* data, size_t size)
        {
            return val.parse(common::ConstDataBuf{data,size});
        }
    ,
    ObjectId::Length);
}

HATN_DATAUNIT_NAMESPACE_END

#endif // HATNOBJECTID_H
