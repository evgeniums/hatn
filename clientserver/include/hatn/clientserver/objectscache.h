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

#include <hatn/clientserver/models/oid.h>
#include <hatn/clientserver/clientserver.h>

HATN_APP_NAMESPACE_BEGIN
class EventDispatcher;
HATN_APP_NAMESPACE_END

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

template <typename Traits>
class ObjectsCache_p;

template <typename Traits>
class ObjectsCache : public ObjectsCacheConfig,
                     public std::enable_shared_from_this<ObjectsCache<Traits>>
{
    public:

        using ObjectType=typename Traits::ObjectType;
        constexpr static const char* ObjectTypeName=Traits::ObjectTypeName;
        using Value=common::SharedPtr<ObjectType>;

        using FetchCb=std::function<void (const common::Error&, Value)>;

        ObjectsCache(
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

        template <typename Context>
        common::Result<Value> get(
            common::SharedPtr<Context> ctx,
            Key uid,
            bool postFetching=false,
            Subject bySubject={},
            bool keepInLocalDb=true
        );

        template <typename Context>
        void fetch(
            common::SharedPtr<Context> ctx,
            FetchCb callback,
            Key uid,
            Subject bySubject={},
            bool keepInLocalDb=true
        );

        void put(
            Value item,
            bool overwriteOnly=false
        );

        template <typename Context>
        void touch(
            common::SharedPtr<Context> ctx,
            Key uid,
            bool inLocalDb=false
        );

        void remove(
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

        template <typename Context>
        void invokeFetch(
            common::SharedPtr<Context> ctx,
            FetchCb callback,
            Key uid,
            Subject bySubject,
            bool keepInLocalDb
        );

        std::unique_ptr<ObjectsCache_p<Traits>> pimpl;
};

HATN_CLIENT_SERVER_NAMESPACE_END

#endif // HATNOBJECTSCACHE_H
