/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/plugins/rocksdb/rocksdberror.h
  *
  *   RocksDB database error.
  *
  */

/****************************************************************************/

#ifndef HATNROCKSDBERROR_H
#define HATNROCKSDBERROR_H

#include <hatn/common/nativeerror.h>

#include <hatn/logcontext/context.h>

#include <hatn/db/dberror.h>

#include <hatn/db/plugins/rocksdb/rocksdbschemadef.h>

#ifndef ROCKSDB_NAMESPACE
    #define ROCKSDB_NAMESPACE rocksdb
#endif

namespace ROCKSDB_NAMESPACE {
class Status;
}

HATN_ROCKSDB_NAMESPACE_BEGIN

// #define HATN_ROCKSDB_EXTENDED_ERROR
#ifdef HATN_ROCKSDB_EXTENDED_ERROR

class RocksdbError : public common::NativeError
{
    public:

        RocksdbError(
            std::string msg,
            int code,
            int subcode,
            int severity
        ) : NativeError(std::move(msg),code,&db::DbErrorCategory::getCategory()),
            m_subcode(subcode),
            m_severity(severity)
        {}

        int subcode() const noexcept
        {
            return m_subcode;
        }

        int severity() const noexcept
        {
            return m_severity;
        }

    private:

        int m_subcode;
        int m_severity;
};

#else
    using RocksdbError=common::NativeError;
#endif

std::shared_ptr<common::NativeError> HATN_ROCKSDB_SCHEMA_EXPORT makeRocksdbError(const ROCKSDB_NAMESPACE::Status& status);

inline void copyRocksdbError(Error& ec, db::DbError code, const ROCKSDB_NAMESPACE::Status& status)
{
    HATN_CTX_SCOPE_LOCK()

    ec.setNative(code,makeRocksdbError(status));
}

inline Error makeError(db::DbError code, const ROCKSDB_NAMESPACE::Status& status)
{
    return Error{code,makeRocksdbError(status)};
}

HATN_ROCKSDB_NAMESPACE_END

#endif // HATNROCKSDBCLIENT_H
