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

#include <map>
#include <type_traits>

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
            //! Epoch ms of the last confirmation by the authority; NotValidated (0) = never.
            int64_t validatedAtMs=CacheConfig::NotValidated;
            size_t updateSubscriptionId=0;

            Item(Value value, int64_t validatedAtMs=CacheConfig::NotValidated)
                : value(std::move(value)),validatedAtMs(validatedAtMs)
            {}
        };

        //! @todo Critical: use version and index in cache keys
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

#if 1
    //! @todo critical: Fix cache!
    ObjectsCache_p(
                Derived* derived,
                common::Thread* thread,
                size_t ttlSeconds,
                const common::pmr::AllocatorFactory* factory
            )
            : localCache(ttlSeconds*1000,
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
              derived(derived),
              dbModelName(Traits::DbModel)
    {}
#else

        ObjectsCache_p(
            Derived* derived,
            common::Thread* thread,
            size_t ttlSeconds,
            const common::pmr::AllocatorFactory* factory
            )
            : derived(derived),
            localCache(ttlSeconds*100000,
                       factory,
                       thread
                       ),
            serverCache(ttlSeconds*100000,
                        factory,
                        thread
                        ),
            guidCache(ttlSeconds*100000,
                      factory,
                      thread
                      ),
            factory(factory),
            dbModelName(Traits::DbModel)
        {}

#endif
    common::MutexLock locker;
    bool stopped=true;
    Derived* derived;
    std::string dbModelName;

    CacheDbModelsProvider* dbModelProvider=nullptr;

    //! 0 disables staleness.
    int64_t invalidateAfterMs=0;

    static int64_t resolveValidatedAt(int64_t validatedAtMs)
    {
        if (validatedAtMs==CacheConfig::ValidatedNow)
        {
            return common::DateTime::millisecondsSinceEpoch();
        }
        return validatedAtMs;
    }

    bool isStale(int64_t validatedAtMs) const
    {
        if (invalidateAfterMs==0)
        {
            return false;
        }
        if (validatedAtMs<=CacheConfig::NotValidated)
        {
            return true;
        }
        return (common::DateTime::millisecondsSinceEpoch()-validatedAtMs)>invalidateAfterMs;
    }

    //! Validation time recorded on a cache db row, NotValidated when absent.
    template <typename CacheItemT>
    static int64_t rowValidatedAt(const CacheItemT& cacheItem)
    {
        const auto& f=cacheItem->field(cache_object::validated_at);
        if (!f.isSet())
        {
            return CacheConfig::NotValidated;
        }
        return f.value().toEpochMs();
    }

    //! Traits::getDbItem may optionally take the cache topic as its last argument.
    template <typename CallbackT>
    void getDbItem(common::SharedPtr<typename Traits::Context> ctx, CallbackT callback, Uid uid, lib::string_view topic)
    {
        if constexpr (std::is_invocable_v<decltype(&Traits::getDbItem),Derived*,common::SharedPtr<typename Traits::Context>,CallbackT,Uid,lib::string_view>)
        {
            Traits::getDbItem(derived,std::move(ctx),std::move(callback),std::move(uid),topic);
        }
        else
        {
            (void)topic;
            Traits::getDbItem(derived,std::move(ctx),std::move(callback),std::move(uid));
        }
    }

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
        guidCache.unlock();
        serverCache.unlock();
        localCache.unlock();
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
                auto query=HATN_DB_NAMESPACE::makeQuery(
                    uidIdx(),
                    db::where(with_uid_idx::ids,HATN_DB_NAMESPACE::query::in,ids)
                    ,
                    topic
                    );
                return query;
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

    //! Rows matching any of a set of ids, ignoring version/index (used for bulk revision lookups).
    struct IdsQueryBuilder
    {
        auto operator()() const
        {
            auto query=HATN_DB_NAMESPACE::makeQuery(
                uidIdx(),
                db::where(with_uid_idx::ids,HATN_DB_NAMESPACE::query::in,*ids),
                *topic
            );
            // one row per matching uid; the default limit (100) could cut a full batch short
            query.setLimit(0);
            return query;
        }

        std::shared_ptr<std::vector<std::string>> ids;
        std::shared_ptr<std::string> topic;
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
        // owned copy of the topic for the asynchronous delete, see put()
        auto topicHolder=std::make_shared<std::string>(topic);
        db->deleteMany(
            std::move(ctx),
            [callback,topicHolder](auto,auto)
            {
                if (callback)
                {
                    callback();
                }
            },
            pimpl->dbModel(),
            pimpl->dbQuery(uid,*topicHolder),
            nullptr,
            *topicHolder
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
        CacheOptions opt,
        int64_t validatedAtMs
    )
{    
    HATN_CTX_ENTER_SCOPE("objectscache::put")

    validatedAtMs=ObjectsCache_p<Traits,Derived>::resolveValidatedAt(validatedAtMs);
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
    typename ObjectsCache_p<Traits,Derived>::Item localItem{item,validatedAtMs};

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
            // pushItem() keeps an existing entry as it is (map emplace), so replace its value
            auto& inserted=pimpl->localCache.pushItem(localUid,localItem);
            inserted.value=item;
            inserted.validatedAtMs=validatedAtMs;
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
            auto& inserted=pimpl->serverCache.pushItem(serverUid,localItem);
            inserted.value=item;
            inserted.validatedAtMs=validatedAtMs;
        }
        auto guid=uid.global();
        if (guid)
        {
            auto& inserted=pimpl->guidCache.pushItem(uid.global(),localItem);
            inserted.value=item;
            inserted.validatedAtMs=validatedAtMs;
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
            // The write below is asynchronous and both its query and db::Topic only VIEW the
            // topic string: keep an owned copy alive until it completes, or a caller whose topic
            // string dies first (e.g. one owned by a network callback) makes the lookup miss the
            // existing row and create a second one under a garbage topic.
            auto topicHolder=std::make_shared<std::string>(topic);

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

            if (validatedAtMs>CacheConfig::NotValidated)
            {
                auto validatedAt=common::DateTime::fromEpochMs(validatedAtMs);
                obj->setFieldValue(cache_object::validated_at,validatedAt);
                request->emplace_back(
                    HATN_DB_NAMESPACE::update::field(cache_object::validated_at,db::update::set,validatedAt)
                );
            }
            else
            {
                request->emplace_back(
                    HATN_DB_NAMESPACE::update::field(cache_object::validated_at,db::update::unset)
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

            // update or create cache object
            db->findUpdateCreate(
                std::move(ctx),
                [ids=std::move(ids),topicHolder,callback](auto,auto dbResult){
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
                pimpl->dbQuery(uid,*topicHolder),
                std::move(request),
                std::move(obj),
                HATN_DB_NAMESPACE::update::ModifyReturn::After,
                nullptr,
                *topicHolder
            );
        }
        else
        {
            // no db for this topic: memory only, but the caller still gets its completion
            HATN_CTX_LEAVE_SCOPE()
            if (callback)
            {
                callback();
            }
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
        const std::string& bySubject
    )
{
    HATN_CTX_SCOPE("objectscache::get")

    if (opt.cacheInMem())
    {
        pimpl->lock();

        bool found=false;
        auto onExit=[&found,this,ctx,topic,uid,opt]()
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
        HATN_SCOPE_GUARD(onExit)

        // try to find in local IDs cache
        auto localUid=uid.local();
        const auto* item1=pimpl->localCache.getAndTouch(localUid);
        if (item1!=nullptr)
        {
            found=true;
            return Result{item1->value,false,pimpl->isStale(item1->validatedAtMs)};
        }

        // try to find in server cache
        auto serverUid=uid.server();
        const auto* item2=pimpl->serverCache.getAndTouch(serverUid);
        if (item2!=nullptr)
        {
            found=true;
            return Result{item2->value,false,pimpl->isStale(item2->validatedAtMs)};
        }

        // try to find in global cache
        auto globalUid=uid.global();
        const auto* item3=pimpl->guidCache.getAndTouch(globalUid);
        if (item3!=nullptr)
        {
            found=true;
            return Result{item3->value,false,pimpl->isStale(item3->validatedAtMs)};
        }
    }

    // make missed result
    Result result;
    result.missed=true;

    // fetch from controller
    if (postFetching)
    {
        auto topicHolder=std::make_shared<std::string>(topic);
        HATN_NAMESPACE::postAsync(
            "ObjectsCache::postFetching",
            Traits::taskThread(pimpl->derived,ctx),
            ctx,
            [guard=Traits::asyncGuard(pimpl->derived),this,topicHolder,fetchCallback,opt,uid,bySubject](auto ctx)
            {
                auto cb=[fetchCallback,topicHolder](const common::Error& ec, Result result)
                {
                    if (fetchCallback)
                    {
                        fetchCallback(ec,std::move(result));
                    }
                };
                invokeFetch(std::move(ctx),std::move(cb),*topicHolder,std::move(uid),std::move(bySubject),opt);
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
        std::string bySubject,
        CacheOptions opt
    )
{
    auto r=get(ctx,topic,uid,opt,false,{},bySubject);
    if (r.isNull() && r.missed)
    {
        HATN_NAMESPACE::postAsync(
            "objectscache::fetch",
            Traits::taskThread(pimpl->derived,ctx),
            ctx,
            [guard=Traits::asyncGuard(pimpl->derived),this,topic,callback,opt,uid,bySubject=std::move(bySubject)](auto ctx)
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
        std::string bySubject,
        CacheOptions opt
    )
{
    HATN_CTX_SCOPE("objectscache::invokefetch")

    auto asynGuard=Traits::asyncGuard(pimpl->derived);

    // update inmem cache
    auto updateInmem=[ctx,callback,asynGuard,this,uid,opt](auto cacheItem) mutable
    {
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

        HATN_CTX_DEBUG(10,"put object to inmem cache")

        // put item to in-memory cache, carrying over the row's own validation time
        auto result=r.takeValue();
        auto validatedAt=ObjectsCache_p<Traits,Derived>::rowValidatedAt(cacheItem);
        auto stale=pimpl->isStale(validatedAt);
        put(std::move(ctx),
            [callback,result,stale]()
            {
                callback({},Result{std::move(result),false,stale});
            },
            result,
            lib::string_view{},
            uid,
            opt.cache_in_db_off(),
            validatedAt
        );
    };

    auto readLocal=[asynGuard,this,updateInmem](
                      auto&& getAppDb,
                      common::SharedPtr<Context> ctx,
                      FetchCb callback,
                      lib::string_view topic,
                      Uid uid,
                      std::string bySubject,
                      CacheOptions opt
                    )
    {
        auto db=Traits::db(pimpl->derived,ctx,topic);
        if (!opt.cacheInDb() || !db)
        {
            getAppDb(std::move(ctx),callback,topic,std::move(uid),bySubject,opt,false);
            return;
        }

        auto cb=[getAppDb=std::move(getAppDb),asynGuard,this,topic,callback,uid,bySubject=std::move(bySubject),opt,updateInmem](auto ctx, auto dbResult) mutable
        {
            // if null then not found, try to get from app database by traits
            if (dbResult || dbResult->isNull())
            {
                HATN_CTX_DEBUG(10,"cache object not found in db cache")

                getAppDb(std::move(ctx),callback,topic,std::move(uid),std::move(bySubject),opt,false);
                return;
            }

            HATN_CTX_DEBUG(10,"cache object found in db cache")

            auto cacheItem=dbResult->shared();
            if (
                !cacheItem->field(cache_object::data).isSet()
                &&
                !cacheItem->fieldValue(cache_object::deleted)
               )
            {
                if (!uid.local())
                {
                    HATN_CTX_DEBUG(10,"local uid not set")
                    getAppDb(std::move(ctx),callback,topic,std::move(uid),std::move(bySubject),opt,false);
                    return;
                }

                HATN_CTX_DEBUG(10,"read reference data for cache object from app")

                // if cache item does not contain data object then get it from app db by traits
                auto getDbCb=[updateInmem,cacheItem,callback,ctx,
                                uid,topic,getAppDb,bySubject=std::move(bySubject),opt](const common::Error& ec, Value item) mutable
                {
                    if (ec)
                    {
                        //! @todo skip error if object not found, i.e. keep in cache but without data
                        getAppDb(std::move(ctx),callback,topic,std::move(uid),std::move(bySubject),opt,true);
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
                pimpl->getDbItem(ctx,getDbCb,uid,topic);
            }
            else
            {
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
                        std::string bySubject,
                        CacheOptions opt,
                        bool skipTraitsDb
                    )
    {
        if (skipTraitsDb)
        {
            farFetch(std::move(ctx),callback,topic,std::move(uid),std::move(bySubject),opt);
            return;
        }

        auto cb=[farFetch=std::move(farFetch),callback,uid,
                 topic,this,ctx,bySubject=std::move(bySubject),opt](const common::Error& ec, Value object) mutable
        {
            if (ec || !object)
            {
                HATN_CTX_DEBUG(10,"cache object data not found in traits db")
                farFetch(std::move(ctx),callback,topic,std::move(uid),std::move(bySubject),opt);
                return;
            }

            // An item assembled by the traits from the application's own tables is authoritative
            // only when it carries a revision (e.g. the user's own record). One assembled from a
            // secondary copy carries none and is served stale, so the caller revalidates it.
            auto validatedAt=object->field(with_revision::revision).isSet()
                                   ? CacheConfig::ValidatedNow
                                   : CacheConfig::NotValidated;
            auto stale=validatedAt==CacheConfig::NotValidated && pimpl->isStale(validatedAt);
            put(
                std::move(ctx),
                [callback,object,stale]()
                {
                    if (callback)
                    {
                        callback({},Result{std::move(object),false,stale});
                    }
                },
                object,
                topic,
                std::move(uid),
                opt,
                validatedAt
            );
        };
        pimpl->getDbItem(ctx,cb,uid,topic);
    };

    auto farFetch=[asynGuard,this](
                         common::SharedPtr<Context> ctx,
                         FetchCb callback,
                         lib::string_view topic,
                         Uid uid,
                         std::string bySubject,
                         CacheOptions opt
                  )
    {
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
    chain(std::move(ctx),callback,topic,std::move(uid),std::move(bySubject),opt);
}

//--------------------------------------------------------------------------

template <typename Traits, typename Derived>
void ObjectsCache<Traits,Derived>::setInvalidateAfterSeconds(size_t value)
{
    common::MutexScopedLock l{pimpl->locker};
    pimpl->invalidateAfterMs=static_cast<int64_t>(value)*1000;
}

//--------------------------------------------------------------------------

template <typename Traits, typename Derived>
size_t ObjectsCache<Traits,Derived>::invalidateAfterSeconds() const
{
    return static_cast<size_t>(pimpl->invalidateAfterMs/1000);
}

//--------------------------------------------------------------------------

template <typename Traits, typename Derived>
void ObjectsCache<Traits,Derived>::markValidated(
        common::SharedPtr<Context> ctx,
        CompletionCb callback,
        lib::string_view topicView,
        Uid uid,
        CacheOptions opt
    )
{
    auto nowMs=common::DateTime::millisecondsSinceEpoch();

    if (opt.cacheInMem())
    {
        pimpl->lock();
        auto stamp=[nowMs](auto* item)
        {
            if (item!=nullptr)
            {
                item->validatedAtMs=nowMs;
            }
        };
        auto localUid=uid.local();
        if (localUid)
        {
            stamp(pimpl->localCache.item(localUid));
        }
        auto serverUid=uid.server();
        if (serverUid)
        {
            stamp(pimpl->serverCache.item(serverUid));
        }
        auto guid=uid.global();
        if (guid)
        {
            stamp(pimpl->guidCache.item(guid));
        }
        pimpl->unlock();
    }

    using DbT=std::decay_t<decltype(Traits::db(pimpl->derived,ctx,topicView))>;
    auto db=opt.cacheInDb() ? DbT{Traits::db(pimpl->derived,ctx,topicView)} : DbT{};
    if (!db)
    {
        if (callback)
        {
            callback();
        }
        return;
    }

    auto topic=std::make_shared<std::string>(topicView);
    auto validatedAt=common::DateTime::fromEpochMs(nowMs);
    auto request=HATN_DB_NAMESPACE::update::sharedRequest(
        HATN_DB_NAMESPACE::update::field(cache_object::validated_at,db::update::set,validatedAt)
    );
    if (opt.dbTtl()!=0)
    {
        auto expireAt=common::DateTime::currentUtc();
        expireAt.addSeconds(static_cast<int>(opt.dbTtl()));
        request->emplace_back(
            HATN_DB_NAMESPACE::update::field(with_expire::expire_at,db::update::set,expireAt)
        );
    }

    db->updateMany(
        std::move(ctx),
        [callback,topic](auto,auto)
        {
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

//--------------------------------------------------------------------------

template <typename Traits, typename Derived>
void ObjectsCache<Traits,Derived>::knownRevisions(
        common::SharedPtr<Context> ctx,
        RevisionsCb callback,
        lib::string_view topicView,
        std::vector<Uid> uids,
        CacheOptions opt
    )
{
    using ObjectId=HATN_DATAUNIT_NAMESPACE::ObjectId;

    auto revisions=std::make_shared<std::vector<ObjectId>>(uids.size());
    std::vector<size_t> missed;

    auto revisionOf=[](const Value& value)
    {
        if (!value)
        {
            return ObjectId{};
        }
        return value->fieldValue(with_revision::revision);
    };

    // memory tier, without touching LRU positions
    if (opt.cacheInMem())
    {
        pimpl->lock();
        for (size_t i=0;i<uids.size();i++)
        {
            const auto& uid=uids[i];
            const typename ObjectsCache_p<Traits,Derived>::Item* item=nullptr;
            auto localUid=uid.local();
            if (localUid)
            {
                item=pimpl->localCache.item(localUid);
            }
            if (item==nullptr)
            {
                auto serverUid=uid.server();
                if (serverUid)
                {
                    item=pimpl->serverCache.item(serverUid);
                }
            }
            if (item==nullptr)
            {
                auto guid=uid.global();
                if (guid)
                {
                    item=pimpl->guidCache.item(guid);
                }
            }
            if (item!=nullptr && item->value)
            {
                (*revisions)[i]=revisionOf(item->value);
            }
            else
            {
                missed.push_back(i);
            }
        }
        pimpl->unlock();
    }
    else
    {
        for (size_t i=0;i<uids.size();i++)
        {
            missed.push_back(i);
        }
    }

    using DbT=std::decay_t<decltype(Traits::db(pimpl->derived,ctx,topicView))>;
    auto db=(!missed.empty() && opt.cacheInDb()) ? DbT{Traits::db(pimpl->derived,ctx,topicView)} : DbT{};
    if (!db)
    {
        callback(std::move(*revisions));
        return;
    }

    // db tier, one query for all missed uids
    auto topic=std::make_shared<std::string>(topicView);
    auto ids=std::make_shared<std::vector<std::string>>();
    auto missedUids=std::make_shared<std::vector<std::pair<size_t,std::vector<std::string>>>>();
    for (auto idx : missed)
    {
        auto uidIds=uids[idx].ids();
        for (const auto& id : uidIds)
        {
            ids->push_back(id);
        }
        missedUids->emplace_back(idx,std::move(uidIds));
    }
    if (ids->empty())
    {
        callback(std::move(*revisions));
        return;
    }

    auto query=HATN_DB_NAMESPACE::wrapQueryBuilder(
        typename ObjectsCache_p<Traits,Derived>::IdsQueryBuilder{ids,topic},
        *topic
    );
    db->find(
        std::move(ctx),
        [callback,revisions,missedUids,ids,topic](auto, auto dbResult)
        {
            if (dbResult)
            {
                HATN_CTX_ERROR(dbResult.error(),"failed to read revisions of cache objects")
                callback(std::move(*revisions));
                return;
            }

            std::map<std::string,ObjectId> byId;
            for (const auto& dbObj : dbResult.value())
            {
                const auto* row=dbObj.template as<cache_object::managed>();
                if (row->fieldValue(cache_object::deleted) || !row->field(with_revision::revision).isSet())
                {
                    continue;
                }
                auto revision=row->fieldValue(with_revision::revision);
                const auto& rowIds=row->field(with_uid_idx::ids);
                for (size_t i=0;i<rowIds.count();i++)
                {
                    byId.emplace(std::string{rowIds.at(i).stringView()},revision);
                }
            }

            for (const auto& missedUid : *missedUids)
            {
                for (const auto& id : missedUid.second)
                {
                    auto it=byId.find(id);
                    if (it!=byId.end())
                    {
                        (*revisions)[missedUid.first]=it->second;
                        break;
                    }
                }
            }
            callback(std::move(*revisions));
        },
        pimpl->dbModel(),
        std::move(query),
        *topic
    );
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
