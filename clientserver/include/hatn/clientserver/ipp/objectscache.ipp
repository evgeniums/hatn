/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*

*/
/** @file clientserver/ipp/objectscache.ipp
  */

/****************************************************************************/

#ifndef HATNOBJECTSCACHE_IPP
#define HATNOBJECTSCACHE_IPP

#include <hatn/common/locker.h>
#include <hatn/common/cachelruttl.h>

#include <hatn/app/eventdispatcher.h>
#include <hatn/app/apperror.h>

#include <hatn/clientserver/objectscache.h>
#include <hatn/clientserver/objectscache.h>

HATN_CLIENT_SERVER_NAMESPACE_BEGIN

//--------------------------------------------------------------------------

template <typename Traits>
class ObjectsCache_p
{
    public:

        struct Item
        {
            typename Traits::Value value;
            size_t updateSubscriptionId=0;

            Item(typename Traits::Value value) : value(std::move(value))
            {}
        };

        common::CacheLruTtl<common::SharedPtr<topic_object::managed>,
                            typename Traits::Value,
                            std::integral_constant<size_t,ObjectsCacheConfig::DefaultCapacity>,
                            CompareTopicObject>
            localCache;

        common::CacheLruTtl<common::SharedPtr<server_object::managed>,
                            typename Traits::Value,
                            std::integral_constant<size_t,ObjectsCacheConfig::DefaultCapacity>,
                            CompareTopicObject>
            serverCache;

        common::CacheLruTtl<common::SharedPtr<guid::managed>,
                            typename Traits::Value,
                            std::integral_constant<size_t,ObjectsCacheConfig::DefaultCapacity>,
                            CompareTopicObject>
            guidCache;

        std::string eventCategory;
        HATN_APP_NAMESPACE::EventDispatcher* eventDispatcher;
        std::set<size_t> subscriprionIds;

    ObjectsCache_p(
                common::Thread* thread,
                size_t ttlSeconds,
                const common::pmr::AllocatorFactory* factory
            )
        :
          localCache(ttlSeconds*1000,
                  factory,
                  thread
                  ),
          serverCache(ttlSeconds*1000,
                   factory,
                   thread
                 ),
          guidCache(ttlSeconds*1000,
                    factory,
                    thread
                    )
    {}

    common::MutexLock locker;
    bool stopped=true;

    void lock()
    {
        locker.lock();
        localCache.lock();
        serverCache.lock();
        guidCache.lock();
    }

    void unlock()
    {
        localCache.unlock();
        serverCache.unlock();
        guidCache.unlock();
        locker.unlock();
    }
};

//--------------------------------------------------------------------------

template <typename Traits>
ObjectsCache<Traits>::ObjectsCache(
        common::Thread* thread,
        size_t ttlSeconds,
        const common::pmr::AllocatorFactory* factory
    ) : pimpl(std::make_unique<ObjectsCache_p<Traits>>(thread,ttlSeconds,factory))
{}

//--------------------------------------------------------------------------

template <typename Traits>
ObjectsCache<Traits>::~ObjectsCache()
{
}

//--------------------------------------------------------------------------

template <typename Traits>
void ObjectsCache<Traits>::start()
{
    common::MutexScopedLock l{pimpl->locker};

    pimpl->localCache.start();
    pimpl->serverCache.start();
    pimpl->guidCache.start();

    pimpl->stopped=false;
}

//--------------------------------------------------------------------------

template <typename Traits>
void ObjectsCache<Traits>::stop()
{
    common::MutexScopedLock l{pimpl->locker};

    pimpl->localCache.stop();
    pimpl->serverCache.stop();
    pimpl->guidCache.stop();

    pimpl->stopped=true;
}

//--------------------------------------------------------------------------

template <typename Traits>
void ObjectsCache<Traits>::setTtlSeconds(size_t value)
{
    common::MutexScopedLock l{pimpl->locker};

    pimpl->localCache.setTtl(value*1000);
    pimpl->serverCache.setTtl(value*1000);
    pimpl->guidCache.setTtl(value*1000);
}

//--------------------------------------------------------------------------

template <typename Traits>
size_t ObjectsCache<Traits>::ttlSeconds() const
{
    return pimpl->localCache.ttl()/1000;
}

//--------------------------------------------------------------------------

template <typename Traits>
void ObjectsCache<Traits>::setCapacity(size_t value)
{
    common::MutexScopedLock l{pimpl->locker};

    pimpl->localCache.setCapacity(value);
    pimpl->serverCache.setCapacity(value);
    pimpl->guidCache.setCapacity(value);

}

//--------------------------------------------------------------------------

template <typename Traits>
size_t ObjectsCache<Traits>::capacity() const
{
    return pimpl->localCache.capacity();
}

//--------------------------------------------------------------------------

template <typename Traits>
void ObjectsCache<Traits>::setEventCategory(std::string cat)
{
    common::MutexScopedLock l{pimpl->locker};
    pimpl->eventCategory=std::move(cat);
}

//--------------------------------------------------------------------------

template <typename Traits>
std::string ObjectsCache<Traits>::eventCategory() const
{
    common::MutexScopedLock l{pimpl->locker};
    return pimpl->eventCategory;
}

//--------------------------------------------------------------------------

template <typename Traits>
void ObjectsCache<Traits>::setEventDispatcher(HATN_APP_NAMESPACE::EventDispatcher* dispatcher)
{
    common::MutexScopedLock l{pimpl->locker};
    pimpl->eventDispatcher=dispatcher;
}

//--------------------------------------------------------------------------

template <typename Traits>
HATN_APP_NAMESPACE::EventDispatcher* ObjectsCache<Traits>::eventDispatcher() const
{
    common::MutexScopedLock l{pimpl->locker};
    return pimpl->eventDispatcher;
}

//--------------------------------------------------------------------------

template <typename Traits>
void ObjectsCache<Traits>::clear()
{
    {
        common::MutexScopedLock l{pimpl->locker};

        if (pimpl->eventDispatcher!=nullptr)
        {
            for (auto&& subscriptionId : pimpl->subscriprionIds)
            {
                pimpl->eventDispatcher->unsubscribe(subscriptionId);
            }
        }
        pimpl->subscriprionIds.clear();
    }

    pimpl->localCache.clear();
    pimpl->serverCache.clear();
    pimpl->guidCache.clear();
}

//--------------------------------------------------------------------------

template <typename Traits>
template <typename Context>
void ObjectsCache<Traits>::touch(
        common::SharedPtr<Context> ctx,
        Key uid,
        bool inLocalDb
    )
{
    pimpl->lock();
    pimpl->localCache.touch(getUidLocal(uid));
    pimpl->serverCache.touch(getUidServer(uid));
    pimpl->guidCache.touch(getUidGlobal(uid));
    pimpl->unlock();

    if (inLocalDb)
    {
        Traits::touch(std::move(ctx),std::move(uid));
    }
}

//--------------------------------------------------------------------------

template <typename Traits>
void ObjectsCache<Traits>::remove(
        Key uid
    )
{
    pimpl->lock();
    pimpl->localCache.remove(getUidLocal(uid));
    pimpl->serverCache.remove(getUidServer(uid));
    pimpl->guidCache.remove(getUidGlobal(uid));
    pimpl->unlock();
}

//--------------------------------------------------------------------------

template <typename Traits>
void ObjectsCache<Traits>::put(
        Value item,
        bool overwriteOnly
    )
{
    auto uid=Traits::uid(item);

    pimpl->lock();

    if (pimpl->stopped)
    {
        pimpl->unlock();
        return;
    }


    // setup subscription to event of item updating
    typename ObjectsCache_p<Traits>::Item localItem{std::move(item)};
    auto localId=getUidLocal(uid);
    auto prev=pimpl->localCache.item(localId,item);
    if (pimpl->eventDispatcher!=nullptr)
    {
        if (prev!=nullptr)
        {
            // copy subscription ID from existing item to replacement item
            localItem.updateSubscriptionId=prev->updateSubscriptionId;
        }
        else
        {
            if (overwriteOnly)
            {
                // skip because ietm not present in the cache
                return;
            }

            // subscribe to event of item updating
            HATN_APP_NAMESPACE::EventKey eventKey{pimpl->eventCategory};
            eventKey.setOid(localId->fieldValue(topic_object::oid));
            auto topic=localId->fieldValue(topic_object::topic);
            if (!topic.empty())
            {
                eventKey.setTopic(topic);
            }
            auto self=this->shared_from_this();
            auto handler=[self=std::move(self),this,localId](auto,
                                                                   auto,
                                                                   auto event)
            {
                {
                    common::MutexScopedLock l{pimpl->locker};
                    if (pimpl->stopped)
                    {
                        return;
                    }
                }

                if (event->event==HATN_APP_NAMESPACE::EventRemove)
                {
                    remove(localId);
                }
                else if (event->messageTypeName == ObjectTypeName && event->message)
                {
                    auto obj=event->message.template sharedAs<ObjectType>();
                    put(obj);
                }
            };
            localItem.updateSubscriptionId=pimpl->eventDispatcher->subscribe(std::move(handler),std::move(eventKey));

            {
                common::MutexScopedLock l{pimpl->locker};
                pimpl->subscriprionIds.insert(localItem.updateSubscriptionId);
            }
        }

        // set displace handler to unsubscribe from dispatcher when item is deleted
        auto self=this->shared_from_this();
        localItem.setDisplaceHandler(
            [self,this](auto item)
            {
                pimpl->eventDispatcher->unsubscribe(item->updateSubscriptionId);

                {
                    common::MutexScopedLock l{pimpl->locker};
                    pimpl->subscriprionIds.erase(item->updateSubscriptionId);
                }
            }
        );
    }
    pimpl->localCache.pushItem(localId,localItem);

    // push item to the rest caches
    pimpl->serverCache.pushItem(getUidServer(uid),item);
    pimpl->guidCache.pushItem(getUidGlobal(uid),item);
    pimpl->unlock();
}

//--------------------------------------------------------------------------

template <typename Traits>
template <typename Context>
common::Result<common::SharedPtr<typename ObjectsCache<Traits>::ObjectType>>
ObjectsCache<Traits>::get(
        common::SharedPtr<Context> ctx,
        Key uid,
        bool postFetching,
        Subject bySubject,
        bool keepInLocalDb
    )
{
    pimpl->lock();

    // try to find in local cache
    auto locId=getUidLocal(uid);
    const auto* item=pimpl->localCache.item(locId);
    if (item!=nullptr)
    {
        pimpl->unlock();
        return item->value;
    }

    // try to find in server cache
    auto serverId=getUidLocal(uid);
    item=pimpl->serverCache.item(serverId);
    if (item!=nullptr)
    {
        pimpl->unlock();
        return item->value;
    }

    // try to find in global cache
    auto globalId=getUidGlobal(uid);
    item=pimpl->guidCache.item(globalId);
    if (item!=nullptr)
    {
        pimpl->unlock();
        return item->value;
    }

    pimpl->unlock();

    // ferch from controller
    if (postFetching)
    {
        auto cb=[](const common::Error&, Value)
        {
        };
        invokeFetch(std::move(ctx),std::move(cb),std::move(uid),std::move(bySubject),keepInLocalDb);
    }

    // return cache miss
    return HATN_APP_NAMESPACE::appError(HATN_APP_NAMESPACE::AppError::CACHE_MISS);
}

//--------------------------------------------------------------------------

template <typename Traits>
template <typename Context>
void ObjectsCache<Traits>::fetch(
        common::SharedPtr<Context> ctx,
        FetchCb callback,
        Key uid,
        Subject bySubject,
        bool keepInLocalDb
    )
{
    auto r=get(ctx,uid);
    if (r)
    {
        invokeFetch(std::move(ctx),std::move(callback),std::move(uid),std::move(bySubject),keepInLocalDb);
    }
}

//--------------------------------------------------------------------------

template <typename Traits>
template <typename Context>
void ObjectsCache<Traits>::invokeFetch(
        common::SharedPtr<Context> ctx,
        FetchCb callback,
        Key uid,
        Subject bySubject,
        bool keepInLocalDb
    )
{
    auto cb=[self=this->shared_from_this(),this,callback](const common::Error& ec, Value object)
    {
        if (ec)
        {
            if (callback)
            {
                callback(ec,std::move(object));
            }
            return;
        }

        if (object)
        {
            put(std::move(object));
        }

        if (callback)
        {
            callback(ec,std::move(object));
        }
    };

    Traits::fetch(std::move(ctx),std::move(cb),std::move(uid),std::move(bySubject),keepInLocalDb);
}

//--------------------------------------------------------------------------

HATN_CLIENT_SERVER_NAMESPACE_END

#endif // HATNOBJECTSCACHE_IPP
