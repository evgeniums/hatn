/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file db/object.h
  *
  * Contains definition of base db object.
  *
  */

/****************************************************************************/

#ifndef HATNDBOBJECT_H
#define HATNDBOBJECT_H

#include <hatn/common/error.h>
#include <hatn/common/databuf.h>

#include <hatn/dataunit/syntax.h>

#include <hatn/db/db.h>
#include <hatn/db/objectid.h>

HATN_DB_NAMESPACE_BEGIN

HDU_UNIT(obj,
    HDU_FIELD(_id,TYPE_OBJECT_ID,100)
    HDU_FIELD(c,TYPE_DATETIME,101)
    HDU_FIELD(u,TYPE_DATETIME,102)
)

namespace object=obj;

inline std::string ObjectIdFieldName{object::_id.name()};
inline std::string CreatedAtFieldName{object::created_at.name()};
inline std::string UpdatedAtFieldName{object::updated_at.name()};

template <typename ObjectT>
void initObject(ObjectT& obj)
{
    // generate _id
    auto* id=obj.field(object::_id).mutableValue();
    id->generate();

    // set creation time
    auto dt=id->toDatetime();
    obj.field(object::created_at).set(dt);

    // set update time
    obj.field(object::updated_at).set(dt);

    // reset wire data keeper
    obj.resetWireDataKeeper();
}

template <typename ObjectT>
ObjectT makeInitObject()
{
    ObjectT obj;
    initObject(obj);
    return obj;
}

HATN_DB_NAMESPACE_END

#endif // HATNDBOBJECT_H
