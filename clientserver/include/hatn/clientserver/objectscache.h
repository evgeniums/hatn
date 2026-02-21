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

#include <hatn/clientserver/clientserver.h>
#include <hatn/clientserver/models/oid.h>
#include <hatn/clientserver/cacheoptions.h>

HATN_CLIENT_SERVER_NAMESPACE_BEGIN

class CacheDbModelsProvider;

class CacheConfig
{
    public:

        constexpr static size_t DefaultInmemTtlSeconds=300;
        constexpr static size_t DefaultCapacity=100;
        constexpr static const char* DefaultEventCategory="cache";
};

class AbstractCache : public CacheConfig
{
};

struct ObjectsCacheTraits
{
    constexpr static const char* DbModel="cache";

    constexpr static const auto IndexVersionField=hana::false_c;
    constexpr static const auto IndexIndexField=hana::false_c;
};

template <typename Traits, typename Derived>
class ObjectsCache_p;

template <typename Traits, typename Derived>
class ObjectsCache : public AbstractCache
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
