/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/plugins/rocksdb/rocksdbencryption.h
  *
  *   RocksDB encryption using hatn.crypt
  *
  */

/****************************************************************************/

#ifndef HATNROCKSDBENCRYPTION_H
#define HATNROCKSDBENCRYPTION_H

#include "rocksdb/file_system.h"

#include <hatn/common/locker.h>

#include <hatn/crypt/cryptfile.h>

#include <hatn/db/plugins/rocksdb/rocksdbdriver.h>

HATN_ROCKSDB_NAMESPACE_BEGIN

namespace rocksdb=ROCKSDB_NAMESPACE;
namespace crypt=HATN_CRYPT_NAMESPACE;

class HATN_ROCKSDB_EXPORT WithCryptfile
{
    public:

        WithCryptfile(
            const crypt::SymmetricKey* masterKey,
            const crypt::CipherSuite* suite=nullptr,
            common::pmr::AllocatorFactory* factory=common::pmr::AllocatorFactory::getDefault()
        );

        crypt::CryptFile& cryptFile()
        {
            return m_cryptfile;
        }

    protected:

        crypt::CryptFile m_cryptfile;
        common::MutexLock m_mutex;
};

template <typename BaseT>
class EncryptedFile : public WithCryptfile, public BaseT
{
    public:

        using WithCryptfile::WithCryptfile;
};

template <typename BaseT>
class EncryptedFileWithCache : public EncryptedFile<BaseT>
{
    public:

        using EncryptedFile<BaseT>::EncryptedFile;

        rocksdb::IOStatus InvalidateCache(size_t offset, size_t length) override;
};

class HATN_ROCKSDB_EXPORT EncryptedSequentialFile : public EncryptedFileWithCache<rocksdb::FSSequentialFile>
{
public:

    using EncryptedFileWithCache<rocksdb::FSSequentialFile>::EncryptedFileWithCache;

    rocksdb::IOStatus Skip(uint64_t n) override;

    rocksdb::IOStatus Read(size_t n, const rocksdb::IOOptions& options, rocksdb::Slice* result,
                           char* scratch, rocksdb::IODebugContext* dbg) override;
};

template <template <typename> class Templ,typename BaseT>
class EncryptedReadFile : public Templ<BaseT>
{
    public:

        using Templ<BaseT>::Templ;

        virtual rocksdb::IOStatus Read(uint64_t offset, size_t n, const rocksdb::IOOptions& options,
                                       rocksdb::Slice* result, char* scratch,
                                       rocksdb::IODebugContext* dbg) const override;
};

class HATN_ROCKSDB_EXPORT EncryptedRandomAccessFile : public EncryptedReadFile<EncryptedFileWithCache,rocksdb::FSRandomAccessFile>
{
    public:

        using EncryptedReadFile<EncryptedFileWithCache,rocksdb::FSRandomAccessFile>::EncryptedReadFile;

        size_t GetUniqueId(char* id, size_t max_size) const override;
};

template <typename BaseT>
class EncryptedSyncFile : public BaseT
{
    public:

        using BaseT::BaseT;

        rocksdb::IOStatus Flush(const rocksdb::IOOptions& options, rocksdb::IODebugContext* dbg) override;

        rocksdb::IOStatus Sync(const rocksdb::IOOptions& options, rocksdb::IODebugContext* dbg) override;

        rocksdb::IOStatus Fsync(const rocksdb::IOOptions& options, rocksdb::IODebugContext* dbg) override;

        rocksdb::IOStatus Close(const rocksdb::IOOptions& options, rocksdb::IODebugContext* dbg) override;
};

class HATN_ROCKSDB_EXPORT EncryptedWritableFile : public EncryptedSyncFile<EncryptedFileWithCache<rocksdb::FSWritableFile>>
{
    public:

        using EncryptedSyncFile<EncryptedFileWithCache<rocksdb::FSWritableFile>>::EncryptedSyncFile;
        using rocksdb::FSWritableFile::Append;

        rocksdb::IOStatus Append(const rocksdb::Slice& data, const rocksdb::IOOptions& options,
                    rocksdb::IODebugContext* dbg) override;

        uint64_t GetFileSize(const rocksdb::IOOptions& options, rocksdb::IODebugContext* dbg) override;

        rocksdb::IOStatus Truncate(uint64_t size, const rocksdb::IOOptions& options,
                      rocksdb::IODebugContext* dbg) override;
};

class HATN_ROCKSDB_EXPORT EncryptedRandomRWFile : public EncryptedSyncFile<
                                                        EncryptedReadFile<EncryptedFile,rocksdb::FSRandomRWFile>
                                                      >
{
    public:

        using EncryptedSyncFile<
                EncryptedReadFile<EncryptedFile,rocksdb::FSRandomRWFile>
            >::EncryptedSyncFile;

        rocksdb::IOStatus Write(uint64_t offset, const rocksdb::Slice& data, const rocksdb::IOOptions& options,
                       rocksdb::IODebugContext* dbg) override;
};

HATN_ROCKSDB_NAMESPACE_END

#endif // HATNROCKSDBENCRYPTION_H
