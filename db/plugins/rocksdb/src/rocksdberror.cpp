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

namespace {

//! Chain std::errc::no_space_on_device as the cause of a disk-full status, so that
//! Error::errorCondition() and the code string expose the ENOSPC condition
//! instead of leaving it only in the native message text.
void attachNoSpaceCause(common::NativeError& err, const ROCKSDB_NAMESPACE::Status& status)
{
    if (status.IsNoSpace())
    {
        err.setPrevError(makeSystemError(std::errc::no_space_on_device));
    }
}

}

//---------------------------------------------------------------

#ifdef HATN_ROCKSDB_EXTENDED_ERROR

std::shared_ptr<common::NativeError> makeRocksdbError(const ROCKSDB_NAMESPACE::Status& status)
{
    auto err=std::make_shared<RocksdbError>(status.ToString(),
                                                      static_cast<int>(status.code()),
                                                      static_cast<int>(status.subcode()),
                                                      static_cast<int>(status.severity())
                                                    );
    attachNoSpaceCause(*err,status);
    return std::static_pointer_cast<common::NativeError>(std::move(err));
}

#else

std::shared_ptr<common::NativeError> makeRocksdbError(const ROCKSDB_NAMESPACE::Status& status)
{
    auto err=std::make_shared<RocksdbError>(status.ToString(),static_cast<int>(status.code()),&db::DbErrorCategory::getCategory());
    attachNoSpaceCause(*err,status);
    return err;
}

#endif

//---------------------------------------------------------------

HATN_ROCKSDB_NAMESPACE_END
