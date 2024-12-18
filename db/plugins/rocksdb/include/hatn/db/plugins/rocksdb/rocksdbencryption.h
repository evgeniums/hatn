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

#include <rocksdb/file_system.h>
#include <rocksdb/env.h>

#include <hatn/common/locker.h>

#include <hatn/crypt/cryptfile.h>

#include <hatn/db/plugins/rocksdb/rocksdbdriver.h>
#include <hatn/db/plugins/rocksdb/encryptionmanager.h>

HATN_ROCKSDB_NAMESPACE_BEGIN

namespace rocksdb=ROCKSDB_NAMESPACE;

template <typename BaseT>
class EncryptedFile : public crypt::WithCryptfile, public BaseT
{
    public:

        using crypt::WithCryptfile::WithCryptfile;
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

using EncryptedRandomAccessFileBase=EncryptedReadFile<EncryptedFileWithCache,rocksdb::FSRandomAccessFile>;

class HATN_ROCKSDB_EXPORT EncryptedRandomAccessFile : public EncryptedRandomAccessFileBase
{
    public:

        // using BaseT=EncryptedReadFile<EncryptedFileWithCache,rocksdb::FSRandomAccessFile>;
        using EncryptedRandomAccessFileBase::EncryptedRandomAccessFileBase;

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

using EncryptedRandomRWFileBase=EncryptedSyncFile<EncryptedReadFile<EncryptedFile,rocksdb::FSRandomRWFile>>;

class HATN_ROCKSDB_EXPORT EncryptedRandomRWFile : public EncryptedRandomRWFileBase
{
    public:

        using EncryptedRandomRWFileBase::EncryptedRandomRWFileBase;

        rocksdb::IOStatus Write(uint64_t offset, const rocksdb::Slice& data, const rocksdb::IOOptions& options,
                       rocksdb::IODebugContext* dbg) override;
};

class HATN_ROCKSDB_EXPORT EncryptedFileSystem : public rocksdb::FileSystemWrapper
{
    public:

        EncryptedFileSystem(
                std::shared_ptr<RocksdbEncryptionManager> encryptionManager,
                const std::shared_ptr<FileSystem>& base
            ) : rocksdb::FileSystemWrapper(base),
                m_encryptionManager(std::move(encryptionManager))
        {}

        static const char* kClassName() { return "hatn::EncryptedFileSystem"; }

        bool IsInstanceOf(const std::string& name) const override
        {
            if (name == kClassName())
            {
                return true;
            } else
            {
                return FileSystemWrapper::IsInstanceOf(name);
            }
        }

        const char* Name() const override
        {
            return kClassName();
        }

        rocksdb::IOStatus NewSequentialFile(const std::string& fname,
                                   const rocksdb::FileOptions& options,
                                   std::unique_ptr<rocksdb::FSSequentialFile>* result,
                                   rocksdb::IODebugContext* dbg) override;

        rocksdb::IOStatus NewRandomAccessFile(const std::string& fname,
                                     const rocksdb::FileOptions& options,
                                     std::unique_ptr<rocksdb::FSRandomAccessFile>* result,
                                     rocksdb::IODebugContext* dbg) override;

        rocksdb::IOStatus NewWritableFile(const std::string& fname, const rocksdb::FileOptions& options,
                                 std::unique_ptr<rocksdb::FSWritableFile>* result,
                                 rocksdb::IODebugContext* dbg) override;

        rocksdb::IOStatus ReopenWritableFile(const std::string& fname,
                                    const rocksdb::FileOptions& options,
                                    std::unique_ptr<rocksdb::FSWritableFile>* result,
                                    rocksdb::IODebugContext* dbg) override;

        rocksdb::IOStatus ReuseWritableFile(const std::string& fname,
                                   const std::string& old_fname,
                                   const rocksdb::FileOptions& options,
                                   std::unique_ptr<rocksdb::FSWritableFile>* result,
                                   rocksdb::IODebugContext* dbg) override;

        rocksdb::IOStatus NewRandomRWFile(const std::string& fname, const rocksdb::FileOptions& options,
                                 std::unique_ptr<rocksdb::FSRandomRWFile>* result,
                                 rocksdb::IODebugContext* dbg) override;

        rocksdb::IOStatus GetFileSize(const std::string& fname, const rocksdb::IOOptions& options,
                             uint64_t* file_size, rocksdb::IODebugContext* dbg) override;

        virtual rocksdb::IOStatus GetChildrenFileAttributes(
            const std::string& dir, const rocksdb::IOOptions& options,
            std::vector<rocksdb::FileAttributes>* result, rocksdb::IODebugContext* dbg) override;

    private:

        std::shared_ptr<RocksdbEncryptionManager> m_encryptionManager;
};

inline std::unique_ptr<rocksdb::Env> makeEncryptedEnv(
        std::shared_ptr<RocksdbEncryptionManager> encryptionManager
    )
{
    auto fs=std::make_shared<EncryptedFileSystem>(std::move(encryptionManager),rocksdb::Env::Default()->GetFileSystem());
    return rocksdb::NewCompositeEnv(fs);
}

HATN_ROCKSDB_NAMESPACE_END

#endif // HATNROCKSDBENCRYPTION_H
