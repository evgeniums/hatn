/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*
    
*/
/** @file clientserver/cacheoptions.h
  */

/****************************************************************************/

#ifndef HATNCACHEOPTIONS_H
#define HATNCACHEOPTIONS_H

#include <cstddef>
#include <cstdint>

#include <hatn/clientserver/clientserver.h>

HATN_CLIENT_SERVER_NAMESPACE_BEGIN

enum class CacheOptionFlag : uint32_t
{
    SetCacheInDb,
    CacheInDbOn,
    SetCacheInMem,
    CacheInMemOn,
    SetCacheDataInDb,
    CacheDataInDbOn,
    SetTouchDb,
    TouchDbOn,
    SetSkip,
    SkipOn,
    SetSaveLocalData,
    SaveLocalDataOn
};

class CacheOptions
{
    public:

        CacheOptions& cache_in_db_on() noexcept
        {
            m_cacheInDb=true;
            return *this;
        }

        CacheOptions& cache_in_db_off() noexcept
        {
            m_cacheInDb=false;
            return *this;
        }

        void setCacheInDb(bool enable) noexcept
        {
            m_cacheInDb=enable;
        }

        bool cacheInDb() const noexcept
        {
            return m_cacheInDb;
        }

        CacheOptions& cache_in_mem_on() noexcept
        {
            m_cacheInMem=true;
            return *this;
        }

        CacheOptions& cache_in_mem_off() noexcept
        {
            m_cacheInMem=false;
            return *this;
        }

        void setCacheInMem(bool enable) noexcept
        {
            m_cacheInMem=enable;
        }

        bool cacheInMem() const noexcept
        {
            return m_cacheInMem;
        }

        CacheOptions& cache_data_in_db_on() noexcept
        {
            m_cacheDataInDb=true;
            return *this;
        }

        CacheOptions& cache_data_in_db_off() noexcept
        {
            m_cacheDataInDb=false;
            return *this;
        }

        void setCacheDataInDb(bool enable) noexcept
        {
            m_cacheDataInDb=enable;
        }

        bool cacheDataInDb() const noexcept
        {
            return m_cacheDataInDb;
        }

        CacheOptions& touch_db_on() noexcept
        {
            m_touchDb=true;
            return *this;
        }

        CacheOptions& touch_db_off() noexcept
        {
            m_touchDb=false;
            return *this;
        }

        void setTouchDb(bool enable) noexcept
        {
            m_touchDb=enable;
        }

        bool touchDb() const noexcept
        {
            return m_touchDb;
        }

        CacheOptions& cache_in_db_ttl(size_t ttl) noexcept
        {
            m_dbTtlSeconds=ttl;
            return *this;
        }

        size_t dbTtl() const noexcept
        {
            return m_dbTtlSeconds;
        }

        CacheOptions& skip_on() noexcept
        {
            m_skipCache=true;
            return *this;
        }

        CacheOptions& skip_off() noexcept
        {
            m_skipCache=false;
            return *this;
        }

        void setSkip(bool enable) noexcept
        {
            m_skipCache=enable;
        }

        bool skip() const noexcept
        {
            return m_skipCache;
        }

        CacheOptions& save_local_on() noexcept
        {
            m_saveInLocalDb=true;
            return *this;
        }

        CacheOptions& save_local_off() noexcept
        {
            m_saveInLocalDb=false;
            return *this;
        }

        void setSaveLocal(bool enable) noexcept
        {
            m_saveInLocalDb=enable;
        }

        bool saveLocal() const noexcept
        {
            return m_saveInLocalDb;
        }

    private:

        bool m_cacheInDb=true;
        bool m_cacheInMem=true;
        bool m_cacheDataInDb=true;
        bool m_touchDb=true;
        bool m_skipCache=false;
        bool m_saveInLocalDb=true;
        size_t m_dbTtlSeconds=0;
};

HATN_CLIENT_SERVER_NAMESPACE_END

#endif // HATNCACHEOPTIONS_H
