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

#include <hatn/db/plugins/rocksdb/rocksdbschemadef.h>

namespace ROCKSDB_NAMESPACE {
class Status;
}

HATN_ROCKSDB_NAMESPACE_BEGIN

#define HATN_ROCKSDB_EXTENDED_ERROR
#ifdef HATN_ROCKSDB_EXTENDED_ERROR

class RocksdbError : public common::NativeError
{
    public:

        RocksdbError(
            std::string msg,
            int code,
            int subcode,
            int severity
        ) : NativeError(std::move(msg),code),
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

std::shared_ptr<common::NativeError> HATN_ROCKSDB_SCHEMA_EXPORT makeError(const ROCKSDB_NAMESPACE::Status& status);

HATN_ROCKSDB_NAMESPACE_END

#endif // HATNROCKSDBCLIENT_H
