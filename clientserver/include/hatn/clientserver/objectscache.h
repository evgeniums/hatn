/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*
    
*/
/** @file clientserver/objectscache.h
  */

/****************************************************************************/

#ifndef HATNOBJECTSCACHE_H
#define HATNOBJECTSCACHE_H

#include <memory>

#include <hatn/common/thread.h>
#include <hatn/common/pmr/allocatorfactory.h>
#include <hatn/app/app.h>
#include <hatn/app/eventdispatcher.h>

#include <hatn/clientserver/models/oid.h>
#include <hatn/clientserver/clientserver.h>

HATN_CLIENT_SERVER_NAMESPACE_BEGIN

class CacheDbModelsProvider;

class CacheConfig
{
    public:

        constexpr static size_t DefaultInmemTtlSeconds=300;
        constexpr static size_t DefaultCapacity=100;
        constexpr static const char* DefaultEventCategory="cache";
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

struct ObjectsCacheTraits
{
    constexpr static const char* DbModel="cache";
};

template <typename Traits, typename Derived>
class ObjectsCache_p;

template <typename Traits, typename Derived>
class ObjectsCache : public CacheConfig
{
    public:

        using ObjectType=typename Traits::ObjectType;
        constexpr static const char* ObjectTypeName=Traits::ObjectTypeName;
        using Value=common::SharedPtr<ObjectType>;
        using Context=typename Traits::Context;

        struct Result
        {
            Value value;
            bool missed;

            Result(Value value={}, bool missed=false)
                : value(std::move(value)),missed(missed)
            {}

            bool isNull() const
            {
                return value.isNull();
            }
        };

        using FetchCb=std::function<void (const common::Error&, Result)>;
        using CompletionCb=std::function<void ()>;

        ObjectsCache();

        ObjectsCache(
            Derived* derived,
            common::Thread* thread,
            size_t ttlSeconds=DefaultInmemTtlSeconds,
            const common::pmr::AllocatorFactory* factory=common::pmr::AllocatorFactory::getDefault()
        );

        ~ObjectsCache();

        ObjectsCache(const ObjectsCache&)=delete;
        ObjectsCache(ObjectsCache&&)=delete;
        ObjectsCache& operator=(const ObjectsCache&)=delete;
        ObjectsCache& operator=(ObjectsCache&&)=delete;

        void init(
            Derived* derived,
            common::Thread* thread,
            size_t ttlSeconds=DefaultInmemTtlSeconds,
            const common::pmr::AllocatorFactory* factory=common::pmr::AllocatorFactory::getDefault()
        );

        void setDbModelProvider(CacheDbModelsProvider* provider);

        void start();
        void stop();

        Result get(
            common::SharedPtr<Context> ctx,
            lib::string_view topic={},
            Uid uid={},
            CacheOptions opt={},
            bool postFetching=false,
            FetchCb fetchCallback={},
            Uid bySubject={}
        );

        void fetch(
            common::SharedPtr<Context> ctx,
            FetchCb callback,
            lib::string_view topic,
            Uid uid,            
            Uid bySubject={},
            CacheOptions opt={}
        );

        void put(
            common::SharedPtr<Context> ctx,
            CompletionCb callback,
            Value item,
            lib::string_view topic={},
            Uid uid={},
            CacheOptions opt={}
        );

        void touch(
            common::SharedPtr<Context> ctx,
            CompletionCb callback,
            lib::string_view topic={},
            Uid uid={},
            CacheOptions opt={}
        );

        void remove(
            common::SharedPtr<Context> ctx,
            CompletionCb callback,
            lib::string_view topic={},
            Uid uid={},
            CacheOptions opt={}
        );

        void setTtlSeconds(size_t ttlSecs);

        size_t ttlSeconds() const;

        void setCapacity(size_t value);

        size_t capacity() const;

        void clear();

        void setEventCategory(std::string cat);

        std::string eventCategory() const;

        void setEventDispatcher(HATN_APP_NAMESPACE::EventDispatcher*);

        HATN_APP_NAMESPACE::EventDispatcher* eventDispatcher() const;

    private:

        void invokeFetch(
            common::SharedPtr<Context> ctx,
            FetchCb callback,
            lib::string_view topic,
            Uid uid,
            Uid bySubject,
            CacheOptions opt
        );

        void updateDbExpiration(
            common::SharedPtr<Context> ctx,
            CompletionCb callback,
            lib::string_view topic,
            Uid uid,
            CacheOptions opt
        );

        std::unique_ptr<ObjectsCache_p<Traits,Derived>> pimpl;
};

HATN_CLIENT_SERVER_NAMESPACE_END

#endif // HATNOBJECTSCACHE_H
