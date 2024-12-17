/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file db/plugins/rocksdb/src/ttlcompactionfilter.cpp
  *
  *   RocksDB encryption using hatn.crypt
  *
  */

/****************************************************************************/

#include <hatn/db/plugins/rocksdb/rocksdbencryption.h>

HATN_ROCKSDB_NAMESPACE_BEGIN

/********************** WithCryptfile **************************/

WithCryptfile::WithCryptfile(
        const crypt::SymmetricKey *masterKey,
        const crypt::CipherSuite *suite,
        common::pmr::AllocatorFactory *factory
    ) : m_cryptfile(masterKey,suite,factory)
{}

//---------------------------------------------------------------

static inline rocksdb::IOStatus ioError()
{
    return rocksdb::IOStatus::IOError();
}

#define HATN_RDB_CHECK_EC(ec) \
    if (ec) \
    {\
        return ioError();\
    }

#define HATN_RDB_DONE_EC(ec) \
    if (ec) \
    {\
        return ioError();\
    }\
    return rocksdb::IOStatus::OK();

/********************** EncryptedFileWithCache **************************/

template <typename BaseT>
rocksdb::IOStatus EncryptedFileWithCache<BaseT>::InvalidateCache(size_t /*offset*/, size_t /*length*/)
{
    common::MutexScopedLock l(this->m_mutex);

    auto ec=this->m_cryptfile.invalidateCache();
    HATN_RDB_DONE_EC(ec)
}

/********************** EncryptedSequentialFile **************************/

rocksdb::IOStatus EncryptedSequentialFile::Skip(uint64_t n)
{
    common::MutexScopedLock l(m_mutex);

    n=std::min(static_cast<size_t>(n),m_cryptfile.size());
    auto ec=m_cryptfile.seek(n);
    HATN_RDB_DONE_EC(ec)
}

//---------------------------------------------------------------

rocksdb::IOStatus EncryptedSequentialFile::Read(
        size_t n,
        const rocksdb::IOOptions &/*options*/,
        rocksdb::Slice *result,
        char *scratch,
        rocksdb::IODebugContext */*dbg*/
    )
{
    common::MutexScopedLock l(m_mutex);

    Error ec;
    auto readSize=m_cryptfile.read(scratch,n,ec);
    HATN_RDB_CHECK_EC(ec)
    rocksdb::Slice r{scratch,readSize};
    *result=r;
    return rocksdb::IOStatus::OK();
}

/********************** EncryptedReadFile **************************/

template <template <typename> class Templ,typename BaseT>
rocksdb::IOStatus EncryptedReadFile<Templ,BaseT>::Read(
        uint64_t offset,
        size_t n,
        const rocksdb::IOOptions &/*options*/,
        rocksdb::Slice *result,
        char *scratch,
        rocksdb::IODebugContext */*dbg*/
    ) const
{
    auto self=const_cast<EncryptedReadFile<Templ,BaseT>*>(this);

    common::MutexScopedLock l(self->m_mutex);

    Error ec;
    ec=self->m_cryptfile.seek(offset);
    HATN_RDB_CHECK_EC(ec)
    auto readSize=self->m_cryptfile.read(scratch,n,ec);
    HATN_RDB_CHECK_EC(ec)
    rocksdb::Slice r{scratch,readSize};
    *result=r;
    return rocksdb::IOStatus::OK();
}

/********************** EncryptedRandomAccessFile **************************/

size_t EncryptedRandomAccessFile::GetUniqueId(char* /*id*/, size_t /*max_size*/) const
{
    //! @todo Use code from rocksdb for posix platforms
    return 0;
}

/********************** EncryptedSyncFile **************************/

template <typename BaseT>
rocksdb::IOStatus EncryptedSyncFile<BaseT>::Flush(
    const rocksdb::IOOptions &/*options*/,
    rocksdb::IODebugContext */*dbg*/
    )
{
    common::MutexScopedLock l(this->m_mutex);

    auto ec=this->m_cryptfile.flush();
    HATN_RDB_DONE_EC(ec)
}

//---------------------------------------------------------------

template <typename BaseT>
rocksdb::IOStatus EncryptedSyncFile<BaseT>::Sync(
    const rocksdb::IOOptions &/*options*/,
    rocksdb::IODebugContext */*dbg*/
    )
{
    common::MutexScopedLock l(this->m_mutex);

    auto ec=this->m_cryptfile.sync();
    HATN_RDB_DONE_EC(ec)
}

//---------------------------------------------------------------

template <typename BaseT>
rocksdb::IOStatus EncryptedSyncFile<BaseT>::Fsync(
    const rocksdb::IOOptions &/*options*/,
    rocksdb::IODebugContext */*dbg*/
    )
{
    common::MutexScopedLock l(this->m_mutex);

    auto ec=this->m_cryptfile.fsync();
    HATN_RDB_DONE_EC(ec)
}

//---------------------------------------------------------------

template <typename BaseT>
rocksdb::IOStatus EncryptedSyncFile<BaseT>::Close(
    const rocksdb::IOOptions &/*options*/,
    rocksdb::IODebugContext */*dbg*/
    )
{
    common::MutexScopedLock l(this->m_mutex);

    Error ec;
    this->m_cryptfile.close(ec);
    HATN_RDB_DONE_EC(ec)
}

/********************** EncryptedWritableFile **************************/

rocksdb::IOStatus EncryptedWritableFile::Append(
        const rocksdb::Slice& data,
        const rocksdb::IOOptions& /*options*/,
        rocksdb::IODebugContext* /* dbg*/
    )
{
    common::MutexScopedLock l(m_mutex);

    //! @todo optimization: Use non-virtual methods od CryptFile

    Error ec;
    auto pos=m_cryptfile.pos();
    auto size=m_cryptfile.size();
    if (pos!=size)
    {
        ec=m_cryptfile.seek(size);
        HATN_RDB_CHECK_EC(ec)
    }
    m_cryptfile.write(data.data(),data.size(),ec);
    HATN_RDB_DONE_EC(ec)
}

//---------------------------------------------------------------

uint64_t EncryptedWritableFile::GetFileSize(
        const rocksdb::IOOptions& /*options*/,
        rocksdb::IODebugContext* /* dbg*/
    )
{
    common::MutexScopedLock l(m_mutex);

    return m_cryptfile.size();
}

//---------------------------------------------------------------

rocksdb::IOStatus EncryptedWritableFile::Truncate(
        uint64_t size,
        const rocksdb::IOOptions &/*options*/,
        rocksdb::IODebugContext */*dbg*/
    )
{
    //! @todo optimization: Expensive operation, maybe disable it

    common::MutexScopedLock l(m_mutex);

    auto ec=m_cryptfile.truncate(size);
    HATN_RDB_DONE_EC(ec)
}

/********************** EncryptedRandomRWFile **************************/

rocksdb::IOStatus EncryptedRandomRWFile::Write(
    uint64_t offset,
    const rocksdb::Slice& data,
    const rocksdb::IOOptions& /*options*/,
    rocksdb::IODebugContext* /*dbg*/)
{
    common::MutexScopedLock l(m_mutex);

    auto ec=m_cryptfile.seek(offset);
    HATN_RDB_CHECK_EC(ec)

    m_cryptfile.write(data.data(),data.size(),ec);
    HATN_RDB_DONE_EC(ec)
}

//---------------------------------------------------------------

HATN_ROCKSDB_NAMESPACE_END
