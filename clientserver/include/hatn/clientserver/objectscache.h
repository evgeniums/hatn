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

class ObjectsCacheConfig
{
    public:

        constexpr static size_t DefaultTtlSeconds=300;
        constexpr static size_t DefaultCapacity=100;
        constexpr static const char* DefaultEventCategory="cache";

        using Key=common::SharedPtr<uid::managed>;
        using Subject=common::SharedPtr<uid::managed>;
};

struct ObjectsCacheTraits
{
    using Key=ObjectsCacheConfig::Key;
    using Subject=ObjectsCacheConfig::Subject;

    constexpr static const char* DbModel="cache";
};

template <typename Traits, typename Derived>
class ObjectsCache_p;

template <typename Traits, typename Derived>
class ObjectsCache : public ObjectsCacheConfig,
                     public std::enable_shared_from_this<ObjectsCache<Traits,Derived>>
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

            operator bool() const
            {
                return value.isNull();
            }
        };

        using FetchCb=std::function<void (const common::Error&, Result)>;

        ObjectsCache(
            Derived* derived,
            common::Thread* thread=common::Thread::currentThreadOrMain(),
            size_t ttlSeconds=DefaultTtlSeconds,
            const common::pmr::AllocatorFactory* factory=common::pmr::AllocatorFactory::getDefault()
        );

        ~ObjectsCache();

        ObjectsCache(const ObjectsCache&)=delete;
        ObjectsCache(ObjectsCache&&)=delete;
        ObjectsCache& operator=(const ObjectsCache&)=delete;
        ObjectsCache& operator=(ObjectsCache&&)=delete;

        void start();
        void stop();

        Result get(
            common::SharedPtr<Context> ctx,
            Key uid,
            bool postFetching=false,            
            Subject bySubject={},
            FetchCb callback={},
            bool keepInLocalDb=true,
            size_t dbTtlSeconds=0
        );

        void fetch(
            common::SharedPtr<Context> ctx,
            FetchCb callback,
            Key uid,
            Subject bySubject={},
            size_t dbTtlSeconds=0
        );

        void put(
            common::SharedPtr<Context> ctx,
            Value item,
            Key uid={},
            bool keepInLocalDb=true,
            size_t dbTtlSeconds=0
        );

        void touch(
            common::SharedPtr<Context> ctx,
            Key uid,
            size_t dbTtlSeconds=0
        );

        void remove(
            common::SharedPtr<Context> ctx,
            Key uid
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
            Key uid,
            Subject bySubject,
            size_t dbTtlSeconds
        );

        std::unique_ptr<ObjectsCache_p<Traits,Derived>> pimpl;
};

HATN_CLIENT_SERVER_NAMESPACE_END

#endif // HATNOBJECTSCACHE_H
