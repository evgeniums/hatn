/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/plugins/rocksdb/rocksdbcreateobject.h
  *
  *   RocksDB database template for object creating.
  *
  */

/****************************************************************************/

#ifndef HATNROCKSDBCREATEOBJECT_H
#define HATNROCKSDBCREATEOBJECT_H

#include <hatn/dataunit/dataunit.h>

#include <hatn/db/namespace.h>
#include <hatn/db/schema.h>

#include <hatn/db/plugins/rocksdb/rocksdbschemadef.h>
#include <hatn/db/plugins/rocksdb/rocksdbhandler.h>

HATN_ROCKSDB_NAMESPACE_BEGIN

struct CreateObjectT
{
    template <typename UnitT, typename ...Indexes>
    void operator ()(RocksdbHandler& handler, const db::Namespace& ns, const db::Model<UnitT,Indexes...>, const UnitT& object, Error& ec) const;
};

constexpr CreateObjectT CreateObject{};

HATN_ROCKSDB_NAMESPACE_END

#endif // HATNROCKSDBCREATEOBJECT_H
