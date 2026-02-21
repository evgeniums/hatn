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
#include <hatn/common/runonscopeexit.h>
#include <hatn/logcontext/postasync.h>
#include <hatn/db/update.h>
#include <hatn/dataunit/wirebufsolid.h>

#include <hatn/app/eventdispatcher.h>
#include <hatn/app/apperror.h>

#include <hatn/clientserver/models/cache.h>
#include <hatn/clientserver/models/cachedbmodel.h>
#include <hatn/clientserver/objectscache.h>

#include <hatn/dataunit/ipp/syntax.ipp>
#include <hatn/dataunit/ipp/objectid.ipp>
#include <hatn/dataunit/ipp/wirebuf.ipp>

#include <hatn/db/ipp/updateunit.ipp>

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

        common::CacheLruTtl<LocalUid,
                            Item,
                            std::integral_constant<size_t,CacheConfig::DefaultCapacity>>
            localCache;

        common::CacheLruTtl<ServerUid,
                            Item,
                            std::integral_constant<size_t,CacheConfig::DefaultCapacity>>
            serverCache;

        common::CacheLruTtl<Guid,
                            Item,
                            std::integral_constant<size_t,CacheConfig::DefaultCapacity>>
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
              dbModelName(Traits::DbModel)
    {}

    common::MutexLock locker;
    bool stopped=true;
    Derived* derived;
    std::string dbModelName;

    CacheDbModelsProvider* dbModelProvider=nullptr;

    auto& dbModel()
    {
        auto m=dbModelProvider->model(dbModelName);
        Assert(m!=nullptr,"Cache database collection not registered in CacheDbModelsProvider, use CacheDbModelsProvider::initCollections() on application startup");
        return *m;
    }

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

    struct QueryBuilder
    {
        auto operator()() const
        {
            if constexpr (Traits::IndexVersionField.value)
            {
                if constexpr (Traits::IndexIndexField.value)
                {
                    auto query=HATN_DB_NAMESPACE::makeQuery(
                        uidIdx(),
                        db::where(with_uid_idx::ids,HATN_DB_NAMESPACE::query::in,ids).
                        and_(db::field(with_uid::uid,uid::version),db::query::eq,uid.version()).
                        and_(db::field(with_uid::uid,uid::index),db::query::eq,uid.index())
                        ,
                        topic
                        );
                    return query;
                }
                else
                {
                    auto query=HATN_DB_NAMESPACE::makeQuery(
                        uidIdx(),
                        db::where(with_uid_idx::ids,HATN_DB_NAMESPACE::query::in,ids).
                        and_(db::field(with_uid::uid,uid::version),db::query::eq,uid.version())
                        ,
                        topic
                        );
                    return query;
                }
            }
            else
            {
                if constexpr (Traits::IndexIndexField.value)
                {
                    auto query=HATN_DB_NAMESPACE::makeQuery(
                        uidIdx(),
                        db::where(with_uid_idx::ids,HATN_DB_NAMESPACE::query::in,ids).
                        and_(db::field(with_uid::uid,uid::index),db::query::eq,uid.index())
                        ,
                        topic
                        );
                    return query;
                }
                else
                {
                    auto query=HATN_DB_NAMESPACE::makeQuery(
                        uidIdx(),
                        db::where(with_uid_idx::ids,HATN_DB_NAMESPACE::query::in,ids)
                        ,
                        topic
                        );
                    return query;
                }
            }
        }

        QueryBuilder(Uid uid, lib::string_view topic) : uid(std::move(uid)), topic(topic)
        {
            ids=this->uid.ids();
        }

        Uid uid;
        lib::string_view topic;
        std::vector<std::string> ids;
    };

    auto dbQuery(Uid uid, lib::string_view topic) const
    {
        auto q=HATN_DB_NAMESPACE::wrapQueryBuilder(
            QueryBuilder{
                std::move(uid),
                topic
            },
            topic
        );
        return q;
    }
};

//--------------------------------------------------------------------------

template <typename Traits, typename Derived>
ObjectsCache<Traits,Derived>::ObjectsCache()
{}

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
void ObjectsCache<Traits,Derived>::init(
        Derived* derived,
        common::Thread* thread,
        size_t ttlSeconds,
        const common::pmr::AllocatorFactory* factory
    )
{
    pimpl=std::make_unique<ObjectsCache_p<Traits,Derived>>(derived,thread,ttlSeconds,factory);
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
        CompletionCb callback,
        lib::string_view topic,
        Uid uid,
        CacheOptions opt
    )
{
    if (opt.cacheInMem())
    {
        pimpl->lock();
        pimpl->localCache.touch(uid.local());
        pimpl->serverCache.touch(uid.server());
        pimpl->guidCache.touch(uid.global());
        pimpl->unlock();
    }

    // touch item in database
    updateDbExpiration(std::move(ctx),callback,topic,uid,opt);
}

//--------------------------------------------------------------------------

template <typename Traits, typename Derived>
void ObjectsCache<Traits,Derived>::updateDbExpiration(
        common::SharedPtr<Context> ctx,
        CompletionCb callback,
        lib::string_view topicView,
        Uid uid,
        CacheOptions opt
    )
{
    if (opt.touchDb() && opt.cacheInDb() && opt.dbTtl()!=0)
    {
        auto topic=std::make_shared<std::string>(topicView);
        auto db=Traits::db(pimpl->derived,ctx,*topic);
        if (!db)
        {
            if (callback)
            {
                callback();
            }
            return;
        }
        HATN_NAMESPACE::postAsync(
            "cbjectscache::updatedbexpiration",
            Traits::taskThread(pimpl->derived,ctx),
            ctx,
            [guard=Traits::asyncGuard(pimpl->derived),this,topic,callback,opt,uid](auto ctx)
            {
                HATN_CTX_DEBUG(10,"objectscache::updatedbexp begin")

                auto db=Traits::db(pimpl->derived,ctx,*topic);

                auto expireAt=common::DateTime::currentUtc();
                expireAt.addSeconds(opt.dbTtl());
                auto request=HATN_DB_NAMESPACE::update::sharedRequest(
                    HATN_DB_NAMESPACE::update::field(with_expire::expire_at,db::update::set,expireAt)
                );

                db->updateMany(
                    std::move(ctx),
                    [callback,topic](auto,auto)
                    {
                        HATN_CTX_DEBUG(10,"objectscache::updatedbexp end")
                        if (callback)
                        {
                            callback();
                        }
                    },
                    pimpl->dbModel(),
                    pimpl->dbQuery(uid,*topic),
                    std::move(request),
                    nullptr,
                    *topic
                );
            }
        );
    }
    else if (callback)
    {
        callback();
    }
}

//--------------------------------------------------------------------------

template <typename Traits, typename Derived>
void ObjectsCache<Traits,Derived>::remove(
        common::SharedPtr<Context> ctx,
        CompletionCb callback,
        lib::string_view topic,
        Uid uid,
        CacheOptions opt
    )
{
    if (opt.cacheInMem())
    {
        pimpl->lock();
        pimpl->localCache.remove(uid.local());
        pimpl->serverCache.remove(uid.server());
        pimpl->guidCache.remove(uid.global());
        pimpl->unlock();
    }

    // remove from database
    auto db=Traits::db(pimpl->derived,ctx,topic);
    if (db && opt.cacheInDb())
    {
        db->deleteMany(
            std::move(ctx),
            [callback](auto,auto)
            {
                if (callback)
                {
                    callback();
                }
            },
            pimpl->dbModel(),
            pimpl->dbQuery(uid,topic),
            nullptr,
            topic
        );
    }
    else if (callback)
    {
        callback();
    }
}

//--------------------------------------------------------------------------

template <typename Traits, typename Derived>
void ObjectsCache<Traits,Derived>::put(
        common::SharedPtr<Context> ctx,
        CompletionCb callback,
        Value item,
        lib::string_view topic,
        Uid uid,
        CacheOptions opt
    )
{    
    HATN_CTX_ENTER_SCOPE("objectscache::put")
    HATN_CTX_DEBUG(10,"objectscache::put")

    if (item && !uid)
    {
        uid=item->field(with_uid::uid).sharedValue();
    }

    auto deleted=!item;
    if (deleted)
    {
        HATN_CTX_DEBUG(10,"cache reference object deleted")
    }
    if (deleted && !uid)
    {
        HATN_CTX_LEAVE_SCOPE()
        if (callback)
        {
            callback();
        }
        return;
    }

    pimpl->lock();

    if (pimpl->stopped)
    {
        pimpl->unlock();

        HATN_CTX_LEAVE_SCOPE()
        if (callback)
        {
            callback();
        }
        return;
    }

    // setup subscription to event of item updating
    typename ObjectsCache_p<Traits,Derived>::Item localItem{item};

    auto localUid=uid.local();
//! @todo Implement cache event handling
#if 0
    HATN_APP_NAMESPACE::EventKey eventKey{pimpl->eventCategory};
    auto localUid=uid.local();
    if (localUid && pimpl->eventDispatcher!=nullptr)
    {
        // subscribe to updates of local item

        auto prev=pimpl->localCache.item(localUid);
        if (prev!=nullptr)
        {
            // copy subscription ID from existing item to replacement item
            localItem.updateSubscriptionId=prev->updateSubscriptionId;
        }
        else
        {
            // subscribe to event of item updating
            eventKey.setOid(localUid.oid()->toString());
            std::string topic{localUid.topic()};
            if (!topic.empty())
            {
                eventKey.setTopic(std::move(topic));
            }
            auto asynGuard=Traits::asyncGuard(pimpl->derived);
            auto handler=[asynGuard=std::move(asynGuard),this,dbTtlSeconds,uid](auto,
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
    auto inserted=pimpl->localCache.pushItem(localUid,localItem);
    if (pimpl->eventDispatcher!=nullptr)
    {
        // set displace handler to unsubscribe from dispatcher when item is deleted
        auto asynGuard=Traits::asyncGuard(pimpl->derived);
        inserted.setDisplaceHandler(
            [asynGuard,this](auto* item)
            {
                pimpl->eventDispatcher->unsubscribe(item->updateSubscriptionId);

                {
                    common::MutexScopedLock l{pimpl->locker};
                    pimpl->subscriprionIds.erase(item->updateSubscriptionId);
                }
            }
        );
    }

#else

    if (opt.cacheInMem())
    {
        HATN_CTX_DEBUG(10,"put cache object to inmem cache")
        if (localUid)
        {
            pimpl->localCache.pushItem(localUid,localItem);
        }
    }
    else
    {
        HATN_CTX_DEBUG(10,"opt not to put cache object to inmem cache")
    }

#endif

    if (opt.cacheInMem())
    {
        // push item to the rest caches
        auto serverUid=uid.server();
        if (serverUid)
        {
            pimpl->serverCache.pushItem(serverUid,item);
        }
        auto guid=uid.global();
        if (guid)
        {
            pimpl->guidCache.pushItem(uid.global(),item);
        }
    }

    // unlock cache
    pimpl->unlock();

    // write item to database
    if (opt.cacheInDb())
    {
        auto db=Traits::db(pimpl->derived,ctx,topic);
        if (db)
        {
            HATN_CTX_STACK_BARRIER_ON("objectscache::put")
            HATN_CTX_STACK_BARRIER_ON("[saveindbcache]")

            HATN_CTX_DEBUG(10,"save cache object in db")

            // fill cache object
            auto obj=pimpl->factory->template createObject<cache_object::managed>();
            HATN_DB_NAMESPACE::initObject(*obj);

            obj->field(with_uid::uid).set(uid.sharedValue());

            if (item)
            {
                if (opt.cacheDataInDb())
                {
                    obj->mutableField(cache_object::data).set(item);
                }
                obj->setFieldValue(cache_object::data_type,item->name());
            }
            else
            {
                obj->setFieldValue(cache_object::deleted,true);
            }

            for (const auto& id : uid.ids())
            {
                obj->field(with_uid_idx::ids).append(id);
            }

            auto request=HATN_DB_NAMESPACE::update::sharedRequest();

            auto expireAt=common::DateTime::currentUtc();
            if (opt.dbTtl()!=0)
            {
                expireAt.addSeconds(opt.dbTtl());
                obj->setFieldValue(with_expire::expire_at,expireAt);
                request->emplace_back(
                    HATN_DB_NAMESPACE::update::field(with_expire::expire_at,db::update::set,expireAt)
                );
            }
            else
            {
                request->emplace_back(
                    HATN_DB_NAMESPACE::update::field(with_expire::expire_at,db::update::unset)
                );
            }

            // fill object update            
            if (item)
            {
                request->emplace_back(
                    HATN_DB_NAMESPACE::update::field(with_revision::revision,db::update::set,item->fieldValue(with_revision::revision))
                );

                if (opt.cacheDataInDb())
                {
                    HATN_DATAUNIT_NAMESPACE::WireBufSolidShared wbuf{pimpl->factory};
                    hatn::Error ec;
                    HATN_DATAUNIT_NAMESPACE::io::serialize(*item,wbuf,ec);
                    if (ec)
                    {
                        HATN_CTX_ERROR(ec,"failed to serialize cache object item's data")
                    }
                    else
                    {
                        item->setSerializedDataHolder(wbuf.sharedMainContainer());
                    }

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
                    HATN_DB_NAMESPACE::update::field(cache_object::data_type,db::update::set,item->name())
                );
            }
            else
            {
                request->emplace_back(
                    HATN_DB_NAMESPACE::update::field(cache_object::data,db::update::unset)
                );
                request->emplace_back(
                    HATN_DB_NAMESPACE::update::field(cache_object::data_type,db::update::unset)
                );
            }

            auto ids=std::make_shared<std::vector<std::string>>(uid.ids());
            request->emplace_back(
                HATN_DB_NAMESPACE::update::field(with_uid_idx::ids,db::update::set,std::cref(*ids))
            );
            HATN_DB_NAMESPACE::update::field(with_uid::uid,db::update::set,uid.sharedValue().template staticCast<HATN_DATAUNIT_NAMESPACE::Unit>());

#if 0
            std::cout << "ObjectsCache::put in cache db " << pimpl->dbModelName
                      << " " << obj->toString(true)
                      <<std::endl;
#endif
            // update or create cache object
            db->findUpdateCreate(
                std::move(ctx),
                [ids=std::move(ids),callback](auto,auto dbResult){
                    if (callback)
                    {
                        HATN_CTX_DEBUG(10,"done saving cache object in db")
                        if (dbResult)
                        {
                            HATN_CTX_ERROR(dbResult.error(),"failed to save cache object")
                        }
                        HATN_CTX_STACK_BARRIER_OFF("objectscache::put")
                        callback();
                    }
                    HATN_CTX_STACK_BARRIER_OFF("objectscache::put")
                },
                pimpl->dbModel(),
                pimpl->dbQuery(uid,topic),
                std::move(request),
                std::move(obj),
                HATN_DB_NAMESPACE::update::ModifyReturn::After,
                nullptr,
                topic
            );
        }
    }
    else
    {
        HATN_CTX_DEBUG(10,"opt not to save cache object in db")
        HATN_CTX_LEAVE_SCOPE()
        if (callback)
        {
            callback();
        }
    }
}

//--------------------------------------------------------------------------

template <typename Traits, typename Derived>
typename ObjectsCache<Traits,Derived>::Result
ObjectsCache<Traits,Derived>::get(
        common::SharedPtr<Context> ctx,
        lib::string_view topic,
        Uid uid,
        CacheOptions opt,
        bool postFetching,
        FetchCb fetchCallback,
        Uid bySubject
    )
{
    HATN_CTX_SCOPE("objectscache::get")

    if (opt.cacheInMem())
    {
        pimpl->lock();

        bool found=false;
        auto ifFound=[&found,this,ctx,topic,uid,opt]()
        {
            pimpl->unlock();
            if (found)
            {
                HATN_CTX_DEBUG(10,"cache object found in memory")

                // touch item in database
                updateDbExpiration(std::move(ctx),
                                   [](){},
                                   topic,uid,opt);
            }
        };
        HATN_SCOPE_GUARD(ifFound)

        // try to find in local IDs cache
        auto localUid=uid.local();
        const auto* item1=pimpl->localCache.getAndTouch(localUid);
        if (item1!=nullptr)
        {
            found=true;
            return item1->value;
        }

        // try to find in server cache
        auto serverUid=uid.server();
        const auto* item2=pimpl->serverCache.getAndTouch(serverUid);
        if (item2!=nullptr)
        {
            found=true;
            return item2->value;
        }

        // try to find in global cache
        auto globalUid=uid.global();
        const auto* item3=pimpl->guidCache.getAndTouch(globalUid);
        if (item3!=nullptr)
        {
            found=true;
            return item3->value;
        }
    }

    // make missed result
    Result result;
    result.missed=true;

    // fetch from controller
    if (postFetching)
    {
        HATN_NAMESPACE::postAsync(
            "ObjectsCache::postFetching",
            Traits::taskThread(pimpl->derived,ctx),
            ctx,
            [guard=Traits::asyncGuard(pimpl->derived),this,topic,fetchCallback,opt,uid,bySubject](auto ctx)
            {
                auto cb=[fetchCallback](const common::Error& ec, Result result)
                {
                    if (fetchCallback)
                    {
                        fetchCallback(ec,std::move(result));
                    }
                };
                invokeFetch(std::move(ctx),std::move(cb),topic,std::move(uid),std::move(bySubject),opt);
            }
        );
    }

    // return cache miss
    return result;
}

//--------------------------------------------------------------------------

template <typename Traits, typename Derived>
void ObjectsCache<Traits,Derived>::fetch(
        common::SharedPtr<Context> ctx,
        FetchCb callback,
        lib::string_view topic,
        Uid uid,
        Uid bySubject,
        CacheOptions opt
    )
{
    auto r=get(ctx,topic,uid,opt);
    if (r.isNull() && r.missed)
    {
        HATN_NAMESPACE::postAsync(
            "objectscache::fetch",
            Traits::taskThread(pimpl->derived,ctx),
            ctx,
            [guard=Traits::asyncGuard(pimpl->derived),this,topic,callback,opt,uid,bySubject](auto ctx)
            {
                invokeFetch(std::move(ctx),std::move(callback),topic,std::move(uid),std::move(bySubject),opt);
            }
        );

        return;
    }

    callback({},std::move(r));
}

//--------------------------------------------------------------------------

template <typename Traits, typename Derived>
void ObjectsCache<Traits,Derived>::invokeFetch(
        common::SharedPtr<Context> ctx,
        FetchCb callback,
        lib::string_view topic,
        Uid uid,
        Uid bySubject,
        CacheOptions opt
    )
{
    HATN_CTX_SCOPE_WITH_BARRIER("[invokefetch]")

    auto asynGuard=Traits::asyncGuard(pimpl->derived);

    // update inmem cache
    auto updateInmem=[ctx,callback,asynGuard,this,topic,uid,opt](auto cacheItem) mutable
    {
        HATN_CTX_STACK_BARRIER_ON("[updateinmem]")
        HATN_CTX_DEBUG(10,"update object in memory cache only")

        if (cacheItem->fieldValue(cache_object::data_type)!=ObjectTypeName)
        {
            HATN_CTX_SCOPE_PUSH("expected_type",ObjectTypeName)
            HATN_CTX_SCOPE_PUSH("actual_type",cacheItem->fieldValue(cache_object::data_type))
            HATN_CTX_SCOPE_ERROR("mismatched data type of cache db object")
            //! @todo use error code like MAILFORMED_INTERNAL_DATA
            auto ec=commonError(CommonError::INVALID_FORMAT);
            callback(ec,{});
            return;
        }

        auto r=HATN_DATAUNIT_NAMESPACE::parseMessageSubunit<ObjectType>(*cacheItem,cache_object::data,pimpl->factory);
        if (r)
        {
            // parsing error
            callback(r.error(),{});
            return;
        }
#if 0
        else
        {
            std::cout << "ObjectsCache::invokeFetch updateinmem parse data ok" << std::endl;
        }
#endif
        HATN_CTX_DEBUG(10,"put object to inmem cache")

        // put item to in-memory cache
        auto result=r.takeValue();
        put(std::move(ctx),
            [callback,result]()
            {
                HATN_CTX_STACK_BARRIER_OFF("[updateinmem]")
                HATN_CTX_STACK_BARRIER_OFF("objectscache::fetch")
                callback({},std::move(result));
            },
            result,
            lib::string_view{},
            uid,
            opt.cache_in_db_off()
        );
    };

    auto readLocal=[asynGuard,this,updateInmem](
                      auto&& getAppDb,
                      common::SharedPtr<Context> ctx,
                      FetchCb callback,
                      lib::string_view topic,
                      Uid uid,
                      Uid bySubject,
                      CacheOptions opt
                    )
    {
        auto db=Traits::db(pimpl->derived,ctx,topic);
        if (!opt.cacheInDb() || !db)
        {
            getAppDb(std::move(ctx),callback,topic,std::move(uid),bySubject,opt,false);
            return;
        }

        HATN_CTX_STACK_BARRIER_ON("[readlocal]")

        auto cb=[getAppDb=std::move(getAppDb),asynGuard,this,topic,callback,uid,bySubject,opt,updateInmem](auto ctx, auto dbResult) mutable
        {
            // if null then not found, try to get from app database by traits
            if (dbResult || dbResult->isNull())
            {
#if 0
                std::cout << "ObjectsCache::invokeFetch not found in cache db " << pimpl->dbModelName << std::endl;
#endif
                HATN_CTX_DEBUG(10,"cache object not found in db cache")
                HATN_CTX_STACK_BARRIER_OFF("[readlocal]")

                getAppDb(std::move(ctx),callback,topic,std::move(uid),bySubject,opt,false);
                return;
            }

            HATN_CTX_DEBUG(10,"cache object found in db cache")

            auto cacheItem=dbResult->shared();
#if 0
            std::cout << "ObjectsCache::invokeFetch found in cache db " << pimpl->dbModelName
                        << " " << cacheItem->toString(true)
                        <<std::endl;
#endif
            if (
                !cacheItem->field(cache_object::data).isSet()
                &&
                !cacheItem->fieldValue(cache_object::deleted)                
               )
            {                
                HATN_CTX_STACK_BARRIER_OFF("[readlocal]")

                if (!uid.local())
                {
#if 0
                    std::cout << "ObjectsCache::invokeFetch local uid not set, trying to read from db by full uid" << std::endl;
#endif
                    HATN_CTX_DEBUG(10,"local uid not set")
                    getAppDb(std::move(ctx),callback,topic,std::move(uid),bySubject,opt,false);
                    return;
                }

                HATN_CTX_SCOPE_WITH_BARRIER("[getdbitem]")
                HATN_CTX_DEBUG(10,"read reference data for cache object from app")

                // if cache item does not contain data object then get it from app db by traits
                auto getDbCb=[updateInmem,cacheItem,callback,ctx,
                                uid,topic,getAppDb,bySubject,opt](const common::Error& ec, Value item) mutable
                {
                    if (ec)
                    {
                        //! @todo skip error if object not found, i.e. keep in cache but without data
                        HATN_CTX_STACK_BARRIER_OFF("objectscache::fetch")
                        getAppDb(std::move(ctx),callback,topic,std::move(uid),bySubject,opt,true);
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

                    HATN_CTX_STACK_BARRIER_OFF("[getdbitem]")
                    updateInmem(cacheItem);
                };
                Traits::getDbItem(pimpl->derived,ctx,getDbCb,uid);
            }
            else
            {
                HATN_CTX_STACK_BARRIER_OFF("[readlocal]")
                updateInmem(cacheItem);
            }
        };

        // find cache object in db
        if (opt.touchDb() && opt.dbTtl()!=0)
        {
            // update expiration
            auto expireAt=common::DateTime::currentUtc();
            expireAt.addSeconds(opt.dbTtl());

            auto request=HATN_DB_NAMESPACE::update::sharedRequest(
                HATN_DB_NAMESPACE::update::field(with_expire::expire_at,db::update::set,expireAt)
            );
            db->findUpdate(
                std::move(ctx),
                std::move(cb),
                pimpl->dbModel(),
                pimpl->dbQuery(uid,topic),
                std::move(request),
                HATN_DB_NAMESPACE::update::ModifyReturn::After,
                nullptr,
                topic
            );
        }
        else
        {
            db->findOne(
                std::move(ctx),
                std::move(cb),
                pimpl->dbModel(),
                pimpl->dbQuery(uid,topic),
                topic
            );
        }
    };

    auto getAppDb=[asynGuard,this](
                        auto&& farFetch,
                        common::SharedPtr<Context> ctx,
                        FetchCb callback,
                        lib::string_view topic,
                        Uid uid,
                        Uid bySubject,
                        CacheOptions opt,
                        bool skipTraitsDb
                    )
    {
        if (skipTraitsDb)
        {
            farFetch(std::move(ctx),callback,topic,std::move(uid),bySubject,opt);
            return;
        }

        HATN_CTX_STACK_BARRIER_ON("[getAppDb]")
        auto cb=[farFetch=std::move(farFetch),callback,uid,
                 topic,this,ctx,bySubject,opt](const common::Error& ec, Value object) mutable
        {
            if (ec || !object)
            {
                HATN_CTX_DEBUG(10,"cache object data not found in traits db")
                HATN_CTX_STACK_BARRIER_OFF("[getAppDb]")
                farFetch(std::move(ctx),callback,topic,std::move(uid),bySubject,opt);
                return;
            }

            put(
                std::move(ctx),
                [callback,object]()
                {
                    HATN_CTX_STACK_BARRIER_OFF("[getAppDb]")
                    if (callback)
                    {
                        callback({},std::move(object));
                    }
                },
                object,
                topic,
                std::move(uid),
                opt
            );
        };
        Traits::getDbItem(pimpl->derived,ctx,cb,uid);
    };

    auto farFetch=[asynGuard,this](
                         common::SharedPtr<Context> ctx,
                         FetchCb callback,
                         lib::string_view topic,
                         Uid uid,
                         Uid bySubject,
                         CacheOptions opt
                  )
    {
        HATN_CTX_STACK_BARRIER_ON("[farfetch]")
        HATN_CTX_DEBUG(10,"invoke far fetch")

        auto cb=[ctx,uid,asynGuardW=common::toWeakPtr(asynGuard),this,callback,opt,topic](const common::Error& ec, Value object) mutable
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
            auto asynGuard=asynGuardW.lock();
            if (asynGuard)
            {
                HATN_CTX_STACK_BARRIER_OFF("farfetch")

                if (object)
                {
                    if (object->field(with_uid::uid).isSet())
                    {
                        Uid newUid{object->field(with_uid::uid).sharedValue()};
                        if (newUid.server())
                        {
                            uid.setServer(newUid.server());
                        }
                        if (newUid.local())
                        {
                            uid.setLocal(newUid.local());
                        }
                    }
                }

                put(
                    std::move(ctx),
                    [callback,object]()
                    {
                        HATN_CTX_STACK_BARRIER_OFF("[invokefetch]")
                        if (callback)
                        {
                            callback({},std::move(object));
                        }
                    },
                    object,
                    topic,
                    std::move(uid),
                    opt
                );
            }
        };

        // invoke far fetch
        Traits::farFetch(pimpl->derived,std::move(ctx),std::move(cb),std::move(uid),std::move(bySubject),topic,opt);
    };

    auto chain=HATN_NAMESPACE::chain(
        std::move(readLocal),
        std::move(getAppDb),
        std::move(farFetch)
    );
    chain(std::move(ctx),callback,topic,std::move(uid),bySubject,opt);
}

//--------------------------------------------------------------------------

template <typename Traits, typename Derived>
void ObjectsCache<Traits,Derived>::setDbModelProvider(CacheDbModelsProvider* provider)
{
    pimpl->dbModelProvider=provider;
}

//--------------------------------------------------------------------------

HATN_CLIENT_SERVER_NAMESPACE_END

#endif // HATNOBJECTSCACHE_IPP
