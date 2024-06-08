/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file db/detail/objectid.ipp
  *
  * Contains definitions of ObjectId template methods.
  *
  */

/****************************************************************************/

#ifndef HATNDBOBJECTID_IPP
#define HATNDBOBJECTID_IPP

#include <hatn/dataunit/rapidjsonsaxhandlers.h>
#include <hatn/dataunit/fieldserialization.h>

#include <hatn/db/objectid.h>

HATN_DB_NAMESPACE_BEGIN

template <typename BufferT>
bool ObjectIdTraits::serialize(const ObjectId& val, BufferT& wired)
{
    auto buf=HATN_DATAUNIT_NAMESPACE::AsBytesSer::serializePrepare(wired,ObjectId::Length);
    val.serialize(buf);
    return true;
}

template <typename BufferT>
bool ObjectIdTraits::deserialize(ObjectId& val, BufferT& wired, HATN_DATAUNIT_NAMESPACE::AllocatorFactory*)
{
    return HATN_DATAUNIT_NAMESPACE::AsBytesSer::deserialize(wired,
        [&val](const char* data, size_t size)
        {
            return val.parse(common::ConstDataBuf{data,size});
        }
    ,
    ObjectId::Length);
}

HATN_DB_NAMESPACE_END

#endif // HATNDBOBJECTID_H
