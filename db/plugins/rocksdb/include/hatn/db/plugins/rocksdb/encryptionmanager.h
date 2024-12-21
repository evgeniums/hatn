/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/plugins/rocksdb/encryptionmanager.h
  *
  */

/****************************************************************************/

#ifndef HATNROCKSDBENCRYPTIONMANAGER_H
#define HATNROCKSDBENCRYPTIONMANAGER_H

#include <boost/algorithm/string/predicate.hpp>

#include <rocksdb/file_system.h>

#include <hatn/db/encryptionmanager.h>

#include <hatn/db/plugins/rocksdb/rocksdbdriver.h>

HATN_ROCKSDB_NAMESPACE_BEGIN

constexpr const size_t RdbDefaultPageSize = 32 * 1024;
constexpr const size_t RdbWalPageSize = 4 * 1024;

class HATN_ROCKSDB_EXPORT RocksdbEncryptionManager
{
    public:

        explicit RocksdbEncryptionManager(
                std::shared_ptr<EncryptionManager> manager,
                uint32_t chunkSize=RdbDefaultPageSize,
                uint32_t walChunkSize=RdbWalPageSize
            ) : m_manager(std::move(manager)),
                m_chunkSize(chunkSize),
                m_walChunkSize(walChunkSize)
        {}

        template <typename T>
        rocksdb::IOStatus createAndOpenFile(
                    const std::string& fname,
                    const rocksdb::FileOptions& options,
                    std::unique_ptr<T>* result,
                    common::File::Mode mode,
                    common::File::ShareMode shareMode
                )
        {
            bool enableCache=true;
            auto chunkSize=m_chunkSize;
            bool streamingMode=false;

            // disable cache for WAL and Log
            // seems like rocksdb ignores the type and always uses IOType::kUnknown
            // so, check file extension as a workaround
            if (options.io_options.type==rocksdb::IOType::kWAL
                ||
                options.io_options.type==rocksdb::IOType::kLog
                ||
                boost::algorithm::ends_with(fname,".log")
                )
            {
                enableCache=false;
                chunkSize=m_walChunkSize;
                if (mode==common::File::Mode::write)
                {
                    mode=common::File::Mode::append;
                    streamingMode=true;
                }
                else
                {
                    mode=common::File::Mode::scan;
                }
            }
#if 0
            std::cout << "createAndOpenFile " << fname << " type=" << int(options.io_options.type) << std::endl;
#endif
            auto ec=m_manager->createAndOpenFile(
                    fname,
                    result,
                    mode,
                    shareMode,
                    enableCache,
                    streamingMode,
                    chunkSize,
                    0
                );
            if (ec)
            {
                if (ec.is(CommonError::FILE_NOT_FOUND))
                {
                    return rocksdb::IOStatus::PathNotFound(ec.message(),fname);
                }
                return rocksdb::IOStatus::IOError(ec.message(),fname);
            }

            // done
            return rocksdb::IOStatus::OK();
        }

        template <typename T>
        rocksdb::IOStatus reopenWritableFile(const std::string& fname,
                                     const rocksdb::FileOptions& options,
                                     std::unique_ptr<T>* result,
                                     common::File::ShareMode shareMode
                                )
        {
            return createAndOpenFile(fname,options,result,common::File::Mode::append,shareMode);
        }

        rocksdb::IOStatus fileSize(const std::string& fname, uint64_t* file_size)
        {
            auto ec=m_manager->fileSize(fname,file_size);
            if (ec)
            {
                if (ec.is(CommonError::FILE_NOT_FOUND))
                {
                    return rocksdb::IOStatus::PathNotFound(ec.message(),fname);
                }
                rocksdb::IOStatus::IOError(ec.message(),fname);
            }
            return rocksdb::IOStatus::OK();
        }

        inline void setChunkMaxSize(uint32_t size) noexcept
        {
            m_chunkSize=size;
        }

        inline uint32_t chunkMaxSize() const noexcept
        {
            return m_chunkSize;
        }

        inline void setWalChunkMaxSize(uint32_t size) noexcept
        {
            m_walChunkSize=size;
        }

        inline uint32_t walChunkMaxSize() const noexcept
        {
            return m_walChunkSize;
        }

    private:

        uint32_t m_chunkSize;
        uint32_t m_walChunkSize;

        std::shared_ptr<EncryptionManager> m_manager;
};

HATN_ROCKSDB_NAMESPACE_END

#endif // HATNROCKSDBENCRYPTIONMANAGER_H
