/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file db/plugins/rocksdb/rocksdberror.cpp
  *
  *   Implementation of RocksDB error helpers.
  *
  */

/****************************************************************************/

#include <memory>

#include <rocksdb/db.h>

#include <hatn/db/plugins/rocksdb/rocksdberror.h>

HATN_ROCKSDB_NAMESPACE_BEGIN

//---------------------------------------------------------------

#ifdef HATN_ROCKSDB_EXTENDED_ERROR

std::shared_ptr<common::NativeError> makeRocksdbError(const ROCKSDB_NAMESPACE::Status& status)
{
    auto err=std::make_shared<RocksdbError>(status.ToString(),
                                                      static_cast<int>(status.code()),
                                                      static_cast<int>(status.subcode()),
                                                      static_cast<int>(status.severity())
                                                    );
    return std::static_pointer_cast<common::NativeError>(std::move(err));
}

#else

std::shared_ptr<common::NativeError> makeRocksdbError(const ROCKSDB_NAMESPACE::Status& status)
{
    return std::make_shared<RocksdbError>(status.ToString(),static_cast<int>(status.code()),&db::DbErrorCategory::getCategory());
}

#endif

//---------------------------------------------------------------

HATN_ROCKSDB_NAMESPACE_END
