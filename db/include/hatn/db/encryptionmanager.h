/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/encryptionmanager.h
  *
  */

/****************************************************************************/

#ifndef HATNDBENCRYPTIONMANAGER_H
#define HATNDBENCRYPTIONMANAGER_H

#include <boost/algorithm/string/predicate.hpp>

#include <hatn/common/locker.h>
#include <hatn/common/file.h>
#include <hatn/common/flatmap.h>
#include <hatn/common/filesystem.h>

#include <hatn/crypt/ciphersuite.h>
#include <hatn/crypt/securekey.h>
#include <hatn/crypt/cryptdataunits.h>
#include <hatn/crypt/cryptfile.h>

#include <hatn/db/db.h>

HATN_DB_NAMESPACE_BEGIN

namespace crypt=HATN_CRYPT_NAMESPACE;

class HATN_DB_EXPORT EncryptionManager
{
    public:

        explicit EncryptionManager(
                const common::pmr::AllocatorFactory* factory=common::pmr::AllocatorFactory::getDefault()
            ) : m_factory(factory),
                m_chunkSize(crypt::MaxContainerChunkSize),
                m_firstChunkSize(crypt::MaxContainerFirstChunkSize)
        {}

        void setSuite(std::shared_ptr<crypt::CipherSuite> suite)
        {
            m_suite=std::move(suite);
        }

        const std::shared_ptr<crypt::CipherSuite>& suite() const
        {
            return m_suite;
        }

        void setDefaultKey(common::SharedPtr<crypt::SymmetricKey> defaultKey)
        {
            m_defaultKey=std::move(defaultKey);
        }

        const common::SharedPtr<crypt::SymmetricKey>& defaultKey() const
        {
            return m_defaultKey;
        }

        inline void setChunkMaxSize(uint32_t size) noexcept
        {
            m_chunkSize=size;
        }

        inline uint32_t chunkMaxSize() const noexcept
        {
            return m_chunkSize;
        }

        inline void setFirstChunkMaxSize(uint32_t size) noexcept
        {
            m_firstChunkSize=size;
        }

        inline uint32_t firstChunkMaxSize() const noexcept
        {
            return m_firstChunkSize;
        }

        void addDb(const std::string& path,common::SharedPtr<crypt::SymmetricKey> dbKey=common::SharedPtr<crypt::SymmetricKey>{})
        {
            common::MutexScopedLock l(m_mutex);
            m_dbPaths.insert(path);
            if (dbKey)
            {
                m_dbKeys.emplace(path,std::move(dbKey));
            }
        }

        void removeDb(const std::string& path)
        {
            common::MutexScopedLock l(m_mutex);
            m_dbPaths.erase(path);
            m_dbKeys.erase(path);
        }

        common::SharedPtr<crypt::SymmetricKey> dbKey(lib::string_view path) const
        {
            common::MutexScopedLock l(m_mutex);

            if (!m_dbKeys.empty())
            {
                auto it=m_dbKeys.find(path);
                if (it!=m_dbKeys.end())
                {
                    return it->second;
                }
            }
            return m_defaultKey;
        }

        void clearDbs()
        {
            common::MutexScopedLock l(m_mutex);
            m_dbPaths.clear();
            m_dbKeys.clear();
        }

        bool hasDb(const std::string& path) const
        {
            common::MutexScopedLock l(m_mutex);
            auto it=m_dbPaths.find(path);
            return it!=m_dbPaths.end();
        }

        std::pair<lib::string_view,lib::string_view> splitPath(const std::string& fname) const
        {
            for (auto&& it: m_dbPaths)
            {
                if (boost::algorithm::starts_with(fname,it))
                {
                    return std::pair<lib::string_view,lib::string_view>{
                        it,
                        lib::string_view{fname.data()+it.size(),fname.size()-it.size()}
                    };
                }
            }
            auto name=lib::filesystem::path(fname).filename();
            if (name.empty())
            {
                return std::pair<lib::string_view,lib::string_view>{lib::string_view{},fname};
            }
            auto nameStr=name.generic_string();
            return std::pair<lib::string_view,lib::string_view>{lib::string_view{fname.data(),fname.size()-nameStr.size()},
                                                                 lib::string_view{fname.data()+fname.size()-nameStr.size(),
                                                                                  nameStr.size()
                                                                                  }
                                                            };
        }

        /**
         * @brief Create and open db file.
         * @param fname File name.
         * @param result Put created file here.
         * @param mode File opening mode.
         * @param enableCache Enable caching in cryptfile.
         * @param chunkSize Chunk size, if empty then use default.
         * @param firstChunkSize First chunk size, if empty then use default.
         * @return Operation status
         *
         * @note T must be derived from WithCryptfile.
         */
        template <typename T>
        Error createAndOpenFile(
            const std::string& fname,
            std::unique_ptr<T>* result,
            common::File::Mode mode,
            common::File::ShareMode shareMode,
            bool enableCache,
            bool streamingMode,
            lib::optional<uint32_t> chunkSize=lib::optional<uint32_t>{},
            lib::optional<uint32_t> firstChunkSize=lib::optional<uint32_t>{}
            )
        {
#if 0
            std::cout << "In EncryptionManager::createAndOpenFile " << fname << " mode=" << int(mode) << std::endl;
#endif
            // split file name to db path and file subpath
            auto nameParts=splitPath(fname);
            auto key=dbKey(nameParts.first);            

            // create file
            auto file=std::make_unique<T>(key.get(),m_suite.get(),m_factory);

            // salt will be auto generated per file
            file->cryptFile().processor().setAutoSalt(true);

            // set HKDF for key derivation
            file->cryptFile().processor().setKdfType(crypt::container_descriptor::KdfType::HKDF);

            // set chunk sizes
            file->cryptFile().processor().setFirstChunkMaxSize(firstChunkSize.value_or(m_firstChunkSize));
            file->cryptFile().processor().setChunkMaxSize(chunkSize.value_or(m_chunkSize));

            // enable/disable cache
            file->cryptFile().setCacheEnabled(enableCache);

            // open file
            file->cryptFile().setShareMode(shareMode);
            if (streamingMode)
            {
                file->cryptFile().setStreamingMode(true);
            }
            auto ec=file->cryptFile().open(fname,mode);
#if 0
            //! @todo Log error
            if (ec)
            {
                std::cout << "EncryptionManager::createAndOpenFile " << fname << " failed: " << ec.message() << std::endl;
            }
#endif
            HATN_CHECK_EC(ec)

            // move file to result
            *result=std::move(file);

            // done
            return OK;
        }

        static Error fileSize(const std::string& fname, uint64_t* size)
        {
#if 0
            std::cout << "In EncryptionManager::fileSize " << fname << std::endl;
#endif
            crypt::CryptFile file;
            file.setShareMode(
                common::File::Sharing::featureMask({
                    common::File::Share::Read,
                    common::File::Share::Write,
                    common::File::Share::Delete
                })
                );
            file.setFilename(fname);
            Error ec;
            *size=file.size(ec);
#if 0
            //! @todo Log error
            if (ec)
            {
                std::cout << "EncryptionManager::fileSize " << fname << " failed: " << ec.message() << std::endl;
            }
#endif
            return ec;
        }

    private:

        std::shared_ptr<crypt::CipherSuite> m_suite;
        common::SharedPtr<crypt::SymmetricKey> m_defaultKey;
        std::map<std::string,common::SharedPtr<crypt::SymmetricKey>,std::less<>> m_dbKeys;

        const common::pmr::AllocatorFactory* m_factory;
        mutable common::MutexLock m_mutex;

        common::FlatSet<std::string> m_dbPaths;

        uint32_t m_chunkSize;
        uint32_t m_firstChunkSize;
};

HATN_DB_NAMESPACE_END

#endif // HATNDBENCRYPTIONMANAGER_H
