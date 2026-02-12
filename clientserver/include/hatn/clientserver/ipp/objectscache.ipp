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
#include <hatn/common/meta/chain.h>

#include <hatn/app/eventdispatcher.h>
#include <hatn/app/apperror.h>

#include <hatn/clientserver/models/cache.h>
#include <hatn/clientserver/models/cachedbmodel.h>
#include <hatn/clientserver/objectscache.h>

#include <hatn/dataunit/ipp/syntax.ipp>
#include <hatn/dataunit/ipp/objectid.ipp>

HATN_CLIENT_SERVER_NAMESPACE_BEGIN

//--------------------------------------------------------------------------

template <typename Traits, typename Derived>
class ObjectsCache_p
{
    public:

        using Value=typename ObjectsCache<Traits,Derived>::Value;

        struct Item
        {
            Value value;
            size_t updateSubscriptionId=0;

            Item(Value value) : value(std::move(value))
            {}
        };

        common::CacheLruTtl<common::SharedPtr<topic_object::managed>,
                            Item,
                            std::integral_constant<size_t,ObjectsCacheConfig::DefaultCapacity>,
                            CompareTopicObject>
            localCache;

        common::CacheLruTtl<common::SharedPtr<server_object::managed>,
                            Item,
                            std::integral_constant<size_t,ObjectsCacheConfig::DefaultCapacity>,
                            CompareServerObject>
            serverCache;

        common::CacheLruTtl<common::SharedPtr<guid::managed>,
                            Item,
                            std::integral_constant<size_t,ObjectsCacheConfig::DefaultCapacity>,
                            CompareGuid>
            guidCache;

        const common::pmr::AllocatorFactory* factory;
        std::string eventCategory;
        HATN_APP_NAMESPACE::EventDispatcher* eventDispatcher;
        std::set<size_t> subscriprionIds;

    ObjectsCache_p(
                Derived* derived,
                common::Thread* thread,
                size_t ttlSeconds,
                const common::pmr::AllocatorFactory* factory
            )
            : derived(derived),
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
                        ),
              factory(factory),
              dbModel(makeCacheModel(Traits::DbModel))
    {}

    common::MutexLock locker;
    bool stopped=true;
    Derived* derived;

    using DbModelType=decltype(makeCacheModel(std::string{}));
    DbModelType dbModel;

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

    auto localObjectQuery(common::SharedPtr<topic_object::managed> locUid) const
    {
        auto q=HATN_DB_NAMESPACE::wrapQueryBuilder(
            [locUid]()
            {
                auto query=HATN_DB_NAMESPACE::makeQuery(
                    cacheLocalIdx(),
                    db::where(cache_object::local_oid,HATN_DB_NAMESPACE::query::eq,locUid->fieldValue(topic_object::oid)),
                    locUid->fieldValue(topic_object::topic)
                    );
                return query;
            },
            locUid->fieldValue(topic_object::topic)
            );
        return q;
    }
};

//--------------------------------------------------------------------------

template <typename Traits, typename Derived>
ObjectsCache<Traits,Derived>::ObjectsCache(
        Derived* derived,
        common::Thread* thread,
        size_t ttlSeconds,
        const common::pmr::AllocatorFactory* factory
    ) : pimpl(std::make_unique<ObjectsCache_p<Traits,Derived>>(derived,thread,ttlSeconds,factory))
{}

//--------------------------------------------------------------------------

template <typename Traits, typename Derived>
ObjectsCache<Traits,Derived>::~ObjectsCache()
{
}

//--------------------------------------------------------------------------

template <typename Traits, typename Derived>
void ObjectsCache<Traits,Derived>::start()
{
    common::MutexScopedLock l{pimpl->locker};

    pimpl->localCache.start();
    pimpl->serverCache.start();
    pimpl->guidCache.start();

    pimpl->stopped=false;
}

//--------------------------------------------------------------------------

template <typename Traits, typename Derived>
void ObjectsCache<Traits,Derived>::stop()
{
    common::MutexScopedLock l{pimpl->locker};

    pimpl->localCache.stop();
    pimpl->serverCache.stop();
    pimpl->guidCache.stop();

    pimpl->stopped=true;
}

//--------------------------------------------------------------------------

template <typename Traits, typename Derived>
void ObjectsCache<Traits,Derived>::setTtlSeconds(size_t value)
{
    common::MutexScopedLock l{pimpl->locker};

    pimpl->localCache.setTtl(value*1000);
    pimpl->serverCache.setTtl(value*1000);
    pimpl->guidCache.setTtl(value*1000);
}

//--------------------------------------------------------------------------

template <typename Traits, typename Derived>
size_t ObjectsCache<Traits,Derived>::ttlSeconds() const
{
    return pimpl->localCache.ttl()/1000;
}

//--------------------------------------------------------------------------

template <typename Traits, typename Derived>
void ObjectsCache<Traits,Derived>::setCapacity(size_t value)
{
    common::MutexScopedLock l{pimpl->locker};

    pimpl->localCache.setCapacity(value);
    pimpl->serverCache.setCapacity(value);
    pimpl->guidCache.setCapacity(value);

}

//--------------------------------------------------------------------------

template <typename Traits, typename Derived>
size_t ObjectsCache<Traits,Derived>::capacity() const
{
    return pimpl->localCache.capacity();
}

//--------------------------------------------------------------------------

template <typename Traits, typename Derived>
void ObjectsCache<Traits,Derived>::setEventCategory(std::string cat)
{
    common::MutexScopedLock l{pimpl->locker};
    pimpl->eventCategory=std::move(cat);
}

//--------------------------------------------------------------------------

template <typename Traits, typename Derived>
std::string ObjectsCache<Traits,Derived>::eventCategory() const
{
    common::MutexScopedLock l{pimpl->locker};
    return pimpl->eventCategory;
}

//--------------------------------------------------------------------------

template <typename Traits, typename Derived>
void ObjectsCache<Traits,Derived>::setEventDispatcher(HATN_APP_NAMESPACE::EventDispatcher* dispatcher)
{
    common::MutexScopedLock l{pimpl->locker};
    pimpl->eventDispatcher=dispatcher;
}

//--------------------------------------------------------------------------

template <typename Traits, typename Derived>
HATN_APP_NAMESPACE::EventDispatcher* ObjectsCache<Traits,Derived>::eventDispatcher() const
{
    common::MutexScopedLock l{pimpl->locker};
    return pimpl->eventDispatcher;
}

//--------------------------------------------------------------------------

template <typename Traits, typename Derived>
void ObjectsCache<Traits,Derived>::clear()
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

template <typename Traits, typename Derived>
void ObjectsCache<Traits,Derived>::touch(
        common::SharedPtr<Context> ctx,
        Key uid,
        size_t dbTtlSeconds
    )
{
    pimpl->lock();
    auto locUid=getUidLocal(uid);
    pimpl->localCache.touch(locUid);
    pimpl->serverCache.touch(getUidServer(uid));
    pimpl->guidCache.touch(getUidGlobal(uid));
    pimpl->unlock();

    if (locUid)
    {
        // touch item in database
        auto db=Traits::db(pimpl->derived,ctx,ObjectsCacheTraits::locIdTopic(uid));
        if (db)
        {
            auto request=HATN_DB_NAMESPACE::update::sharedRequest(
                HATN_DB_NAMESPACE::update::field(cache_object::touch,db::update::set,1)
            );
            if (dbTtlSeconds!=0)
            {
                auto expireAt=common::DateTime::currentUtc();
                expireAt.addSeconds(dbTtlSeconds);
                request->emplace_back(
                    HATN_DB_NAMESPACE::update::field(with_expire::expire_at,db::update::set,expireAt)
                );
            }

            db->updateMany(
                std::move(ctx),
                [](auto,auto){},
                pimpl->dbModel,
                pimpl->localObjectQuery(locUid),
                std::move(request),
                nullptr,
                locUid->fieldValue(topic_object::topic)
            );
        }
    }
}

//--------------------------------------------------------------------------

template <typename Traits, typename Derived>
void ObjectsCache<Traits,Derived>::remove(
        common::SharedPtr<Context> ctx,
        Key uid
    )
{
    pimpl->lock();
    auto locUid=getUidLocal(uid);
    pimpl->localCache.remove(locUid);
    pimpl->serverCache.remove(getUidServer(uid));
    pimpl->guidCache.remove(getUidGlobal(uid));
    pimpl->unlock();

    if (locUid)
    {
        // remove item from database
        auto db=Traits::db(pimpl->derived,ctx,ObjectsCacheTraits::locIdTopic(uid));
        if (db)
        {
            db->deleteMany(
                std::move(ctx),
                [](auto,auto){},
                pimpl->dbModel,
                pimpl->localObjectQuery(locUid),
                nullptr,
                ObjectsCacheTraits::locIdTopic(uid)
            );
        }
    }
}

//--------------------------------------------------------------------------

template <typename Traits, typename Derived>
void ObjectsCache<Traits,Derived>::put(
        common::SharedPtr<Context> ctx,
        Value item,
        Key uid,
        bool keepInLocalDb,
        bool localDbFullObject,
        size_t dbTtlSeconds
    )
{
    if (item && !uid)
    {
        uid=item->field(with_uid::uid).sharedValue();
    }

    auto deleted=!item;
    if (deleted && !uid)
    {
        return;
    }

    pimpl->lock();

    if (pimpl->stopped)
    {
        pimpl->unlock();
        return;
    }

    // setup subscription to event of item updating
    typename ObjectsCache_p<Traits,Derived>::Item localItem{std::move(item)};
    localItem.value=item;

    HATN_APP_NAMESPACE::EventKey eventKey{pimpl->eventCategory};
    auto localId=getUidLocal(uid);
    auto prev=pimpl->localCache.item(localId);
    if (pimpl->eventDispatcher!=nullptr)
    {
        if (prev!=nullptr)
        {
            // copy subscription ID from existing item to replacement item
            localItem.updateSubscriptionId=prev->updateSubscriptionId;
        }
        else
        {
            // subscribe to event of item updating
            eventKey.setOid(localId->fieldValue(topic_object::oid).toString());
            std::string topic{localId->fieldValue(topic_object::topic)};
            if (!topic.empty())
            {
                eventKey.setTopic(std::move(topic));
            }
            auto self=this->shared_from_this();
            auto handler=[self=std::move(self),this,dbTtlSeconds,uid](auto,
                                                                   auto ctx,
                                                                   auto event)
            {
                {
                    common::MutexScopedLock l{pimpl->locker};
                    if (pimpl->stopped)
                    {
                        return;
                    }
                }

                auto opCtx=Traits::makeContext(pimpl->derived,ctx);
                if (event->event==HATN_APP_NAMESPACE::EventRemove)
                {                    
                    remove(opCtx,uid);
                }
                else if (event->messageTypeName == ObjectTypeName && event->message)
                {
                    auto obj=event->message.template sharedAs<ObjectType>();
                    put(opCtx,obj,uid,true,dbTtlSeconds);
                }
            };
            localItem.updateSubscriptionId=pimpl->eventDispatcher->subscribe(std::move(handler),std::move(eventKey));

            {
                common::MutexScopedLock l{pimpl->locker};
                pimpl->subscriprionIds.insert(localItem.updateSubscriptionId);
            }
        }        
    }
    auto inserted=pimpl->localCache.pushItem(localId,localItem);
    if (pimpl->eventDispatcher!=nullptr)
    {
        // set displace handler to unsubscribe from dispatcher when item is deleted
        auto self=this->shared_from_this();
        inserted.setDisplaceHandler(
            [self,this](auto* item)
            {
                pimpl->eventDispatcher->unsubscribe(item->updateSubscriptionId);

                {
                    common::MutexScopedLock l{pimpl->locker};
                    pimpl->subscriprionIds.erase(item->updateSubscriptionId);
                }
            }
        );
    }

    // push item to the rest caches
    auto serverId=getUidServer(uid);
    if (serverId)
    {
        pimpl->serverCache.pushItem(serverId,item);
    }
    auto guid=getUidGlobal(uid);
    if (guid)
    {
        pimpl->guidCache.pushItem(getUidGlobal(uid),item);
    }
    // unlock cache
    pimpl->unlock();

    // write item to database
    if (keepInLocalDb && localId)
    {
        auto db=Traits::db(pimpl->derived,ctx,ObjectsCacheTraits::locIdTopic(uid));
        if (db)
        {
            // fill cache object
            auto obj=pimpl->factory->template createObject<cache_object::managed>();
            HATN_DB_NAMESPACE::initObject(*obj);

            auto localOid=uid->member(uid::local,topic_object::oid);
            if (localOid!=nullptr)
            {
                obj->setFieldValue(cache_object::local_oid,localOid->value());
            }
            else
            {
                // use cache_object's oid as oid if there is no reference object in local database
                obj->setFieldValue(cache_object::local_oid,obj->fieldValue(HATN_DB_NAMESPACE::object::_id));
            }

            auto serverId=uid->field(uid::server).sharedValue();
            if (serverId)
            {
                auto str=serverObjectHash(serverId);
                if (!str.empty())
                {
                    obj->setFieldValue(cache_object::server_hash,str);
                }
            }
            auto globalId=uid->field(uid::global).sharedValue();
            if (globalId)
            {
                auto str=guidObjectHash(globalId);
                if (!str.empty())
                {
                    obj->setFieldValue(cache_object::guid_hash,str);
                }
            }
            if (item)
            {
                if (localDbFullObject)
                {
                    obj->mutableField(cache_object::data).set(item);
                }
                obj->setFieldValue(cache_object::object_type,item->name());
            }
            else
            {
                obj->setFieldValue(cache_object::deleted,true);
            }

            // fill object update
            auto request=HATN_DB_NAMESPACE::update::sharedRequest();
            if (item)
            {
                request->emplace_back(
                    HATN_DB_NAMESPACE::update::field(with_revision::revision,db::update::set,item->fieldValue(with_revision::revision))
                );

                if (localDbFullObject)
                {
                    request->emplace_back(
                        HATN_DB_NAMESPACE::update::field(cache_object::data,db::update::set,item.template staticCast<HATN_DATAUNIT_NAMESPACE::Unit>())
                        );
                }
                else
                {
                    request->emplace_back(
                        HATN_DB_NAMESPACE::update::field(cache_object::data,db::update::unset)
                    );
                }

                request->emplace_back(
                    HATN_DB_NAMESPACE::update::field(cache_object::object_type,db::update::set,item->name())
                );
                if (dbTtlSeconds!=0)
                {
                    auto expireAt=common::DateTime::currentUtc();
                    expireAt.addSeconds(dbTtlSeconds);
                    request->emplace_back(
                        HATN_DB_NAMESPACE::update::field(with_expire::expire_at,db::update::set,expireAt)
                    );
                }
            }
            else
            {
                request->emplace_back(
                    HATN_DB_NAMESPACE::update::field(cache_object::data,db::update::unset)
                );
                request->emplace_back(
                    HATN_DB_NAMESPACE::update::field(cache_object::object_type,db::update::unset)
                );
                if (dbTtlSeconds!=0)
                {
                    request->emplace_back(
                        HATN_DB_NAMESPACE::update::field(with_expire::expire_at,db::update::unset)
                    );
                }
            }

            // update or create cache object
            db->findUpdateCreate(
                std::move(ctx),
                [](auto,auto){},
                pimpl->dbModel,
                pimpl->localObjectQuery(localId),
                std::move(request),
                std::move(obj),
                HATN_DB_NAMESPACE::update::ModifyReturn::After,
                nullptr,
                ObjectsCacheTraits::locIdTopic(uid)
            );
        }
    }
}

//--------------------------------------------------------------------------

template <typename Traits, typename Derived>
typename ObjectsCache<Traits,Derived>::Result
ObjectsCache<Traits,Derived>::get(
        common::SharedPtr<Context> ctx,
        Key uid,
        bool postFetching,
        Subject bySubject,
        FetchCb callback,
        bool localDbFullObject,
        size_t dbTtlSeconds
    )
{
    pimpl->lock();

    // try to find in local cache
    auto locId=getUidLocal(uid);
    const auto* item1=pimpl->localCache.item(locId);
    if (item1!=nullptr)
    {
        pimpl->unlock();
        return item1->value;
    }

    // try to find in server cache
    auto serverId=getUidServer(uid);
    const auto* item2=pimpl->serverCache.item(serverId);
    if (item2!=nullptr)
    {
        pimpl->unlock();
        return item2->value;
    }

    // try to find in global cache
    auto globalId=getUidGlobal(uid);
    const auto* item3=pimpl->guidCache.item(globalId);
    if (item3!=nullptr)
    {
        pimpl->unlock();
        return item3->value;
    }

    pimpl->unlock();

    // make missed result
    Result result;
    result.missed=true;

    // fetch from controller
    if (postFetching)
    {
        auto cb=[callback](const common::Error& ec, Result result)
        {
            if (callback)
            {
                callback(ec,std::move(result));
            }
        };
        invokeFetch(std::move(ctx),std::move(cb),std::move(uid),std::move(bySubject),localDbFullObject,dbTtlSeconds);
    }

    // return cache miss
    return result;
}

//--------------------------------------------------------------------------

template <typename Traits, typename Derived>
void ObjectsCache<Traits,Derived>::fetch(
        common::SharedPtr<Context> ctx,
        FetchCb callback,
        Key uid,
        Subject bySubject,
        bool localDbFullObject,
        size_t dbTtlSeconds
    )
{
    auto r=get(ctx,uid);
    if (r && r.missed)
    {
        invokeFetch(std::move(ctx),std::move(callback),std::move(uid),std::move(bySubject),localDbFullObject,dbTtlSeconds);
        return;
    }

    callback({},std::move(r));
}

//--------------------------------------------------------------------------

template <typename Traits, typename Derived>
void ObjectsCache<Traits,Derived>::invokeFetch(
        common::SharedPtr<Context> ctx,
        FetchCb callback,
        Key uid,
        Subject bySubject,
        bool localDbFullObject,
        size_t dbTtlSeconds
    )
{
    auto self=this->shared_from_this();
    auto readLocal=[self,this,localDbFullObject](
                      auto&& farFetch,
                      common::SharedPtr<Context> ctx,
                      FetchCb callback,
                      Key uid,
                      Subject bySubject,
                      size_t dbTtlSeconds
                    )
    {
        auto locId=getUidLocal(uid);
        auto db=Traits::db(pimpl->derived,ctx,ObjectsCacheTraits::locIdTopic(uid));
        if (!locId || !db)
        {
            farFetch(std::move(ctx),callback,std::move(uid),bySubject,localDbFullObject,dbTtlSeconds);
        }

        auto cb=[farFetch=std::move(farFetch),self,this,callback,uid,bySubject,localDbFullObject,dbTtlSeconds](auto ctx, auto dbResult) mutable
        {
            // handle error
            if (!dbResult)
            {
                callback(dbResult.error(),{});
                return;
            }

            // if null then not found, go to far fetch
            if (dbResult->isNull())
            {
                farFetch(std::move(ctx),callback,std::move(uid),bySubject,localDbFullObject,dbTtlSeconds);
                return;
            }

            // parse data            
            auto cacheItem=dbResult->shared();

            // update inmem cache
            auto updateInmem=[ctx,callback,self,this,uid](auto cacheItem)
            {
                auto r=HATN_DATAUNIT_NAMESPACE::parseMessageSubunit<ObjectType>(*cacheItem,cache_object::data,pimpl->factory);
                if (r)
                {
                    // parsing error
                    callback(r.error(),{});
                    return;
                }

                // put item to in-memory cache
                put(std::move(ctx),r.value(),uid,false);

                // invoke callback
                callback({},Result{r.value()});
            };

            if (cacheItem->field(cache_object::data).isSet() && !cacheItem->fieldValue(cache_object::deleted))
            {
                // if cache item does not contain data object then get it from local db
                auto getDbCb=[updateInmem,cacheItem,callback](const common::Error& ec, Value item) mutable
                {
                    if (ec)
                    {
                        callback(ec,{});
                        return;
                    }

                    if (item)
                    {
                        cacheItem->field(cache_object::data).set(std::move(item));
                        cacheItem->field(cache_object::deleted).set(false);
                    }
                    else
                    {
                        cacheItem->field(cache_object::data).reset();
                        cacheItem->field(cache_object::deleted).set(true);
                    }
                    updateInmem(cacheItem);
                };
                Traits::getDbItem(pimpl->derived,ctx,getDbCb,uid);
            }
            else
            {
                updateInmem(cacheItem);
            }
        };

        // find cache object in db
        db->findOne(
            std::move(ctx),
            std::move(cb),
            pimpl->dbModel,
            pimpl->localObjectQuery(locId),
            ObjectsCacheTraits::locIdTopic(uid)
        );
    };

    auto farFetch=[self,this](
                         common::SharedPtr<Context> ctx,
                         FetchCb callback,
                         Key uid,
                         Subject bySubject,
                         bool localDbFullObject,
                         size_t dbTtlSeconds
                  )
    {
        auto cb=[ctx,uid,selfW=this->weak_from_this(),this,callback,localDbFullObject,dbTtlSeconds](const common::Error& ec, Value object) mutable
        {
            // handle error
            if (ec)
            {
                if (callback)
                {
                    callback(ec,{});
                }
                return;
            }

            // save result in cache
            auto self=selfW.lock();
            if (self)
            {
                if (object && object->field(with_uid::uid).isSet())
                {
                    uid=object->field(with_uid::uid).sharedValue();
                }
                put(std::move(ctx),object,std::move(uid),true,localDbFullObject,dbTtlSeconds);
            }

            // invoke callback
            if (callback)
            {
                callback(ec,std::move(object));
            }
        };

        // invoke far fetch
        Traits::farFetch(pimpl->derived,std::move(ctx),std::move(cb),std::move(uid),std::move(bySubject));
    };

    auto chain=HATN_NAMESPACE::chain(
        std::move(readLocal),
        std::move(farFetch)
    );
    chain(std::move(ctx),callback,std::move(uid),bySubject,dbTtlSeconds);
}

//--------------------------------------------------------------------------

HATN_CLIENT_SERVER_NAMESPACE_END

#endif // HATNOBJECTSCACHE_IPP
