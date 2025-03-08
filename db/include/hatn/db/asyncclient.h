/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/asyncclient.h
  *
  * Contains declaration of async wrapper of database client.
  *
  */

/****************************************************************************/

#ifndef HATNDASYNCBCLIENT_H
#define HATNDASYNCBCLIENT_H

#include <hatn/common/thread.h>
#include <hatn/common/taskcontext.h>

#include <hatn/db/db.h>
#include <hatn/db/client.h>

HATN_DB_NAMESPACE_BEGIN

class HATN_DB_EXPORT AsyncClient :  public common::MappedThreadQWithTaskContext,
                                    public common::TaskSubcontext
{
    public:

        AsyncClient(
                std::shared_ptr<Client> client,
                common::MappedThreadMode threadMode=common::MappedThreadMode::Caller,
                common::ThreadQWithTaskContext* defaultThread=common::ThreadQWithTaskContext::current()
            ) : common::MappedThreadQWithTaskContext(threadMode,defaultThread),
                m_client(std::move(client))
        {}

        ~AsyncClient()=default;

        AsyncClient(const AsyncClient&)=delete;
        AsyncClient(AsyncClient&&)=default;
        AsyncClient& operator=(const AsyncClient&)=delete;
        AsyncClient& operator=(AsyncClient&&)=default;

        std::shared_ptr<ClientEnvironment> cloneEnvironment()
        {
            return m_client->cloneEnvironment();
        }

        bool isOpen() const noexcept
        {            
            return m_client->isOpen();
        }

    private:

        std::shared_ptr<Client> m_client;

    public:

        template <typename ContextT, typename CallbackT>
        void openDb(
                common::SharedPtr<ContextT> ctx,
                CallbackT cb,
                const ClientConfig& config
            )
        {
            common::postAsyncTask(
                thread(),
                ctx,
                [ctx,cb{std::move(cb)},&config,this,selfCtx{sharedMainCtx()}](const common::SharedPtr<common::TaskContext>&)
                {
                    base::config_object::LogRecords records;
                    auto ec=m_client->openDb(config,records);
                    //! @todo Log error records
                    cb(std::move(ctx),std::move(ec));
                }
            );
        }

        template <typename ContextT, typename CallbackT>
        void closeDb(
                common::SharedPtr<ContextT> ctx,
                CallbackT cb
            )
        {
            common::postAsyncTask(
                thread(),
                ctx,
                [ctx,cb{std::move(cb)},this](const common::SharedPtr<common::TaskContext>&)
                {
                    cb(std::move(ctx),m_client->closeDb());
                }
            );
        }

        template <typename ContextT, typename CallbackT>
        void createDb(
                common::SharedPtr<ContextT> ctx,
                CallbackT cb,
                const ClientConfig& config
            )
        {
            common::postAsyncTask(
                thread(),
                ctx,
                [ctx,cb{std::move(cb)},&config,this](const common::SharedPtr<common::TaskContext>&)
                {
                    base::config_object::LogRecords records;
                    auto ec=m_client->createDb(config,records);
                    //! @todo Log error records
                    cb(std::move(ctx),std::move(ec));
                }
            );
        }

        template <typename ContextT, typename CallbackT>
        void destroyDb(
            common::SharedPtr<ContextT> ctx,
            CallbackT cb,
            const ClientConfig& config
            )
        {
            common::postAsyncTask(
                thread(),
                ctx,
                [ctx,cb{std::move(cb)},&config,this](const common::SharedPtr<common::TaskContext>&)
                {
                    base::config_object::LogRecords records;
                    auto ec=m_client->destroyDb(config,records);
                    //! @todo Log error records
                    cb(std::move(ctx),std::move(ec));
                }
            );
        }

        template <typename ContextT, typename CallbackT>
        void setSchema(
            common::SharedPtr<ContextT> ctx,
            CallbackT cb,
            std::shared_ptr<Schema> schema
            )
        {
            common::postAsyncTask(
                thread(),
                ctx,
                [ctx,cb{std::move(cb)},schema{std::move(schema)},this](const common::SharedPtr<common::TaskContext>&)
                {
                    cb(std::move(ctx),m_client->setSchema(std::move(schema)));
                }
            );
        }

        template <typename ContextT, typename CallbackT>
        void schema(
                common::SharedPtr<ContextT> ctx,
                CallbackT cb
            )
        {
            common::postAsyncTask(
                thread(),
                ctx,
                [ctx,cb{std::move(cb)},this](const common::SharedPtr<common::TaskContext>&)
                {
                    cb(std::move(ctx),m_client->schema());
                }
            );
        }

        template <typename ContextT, typename CallbackT>
        void checkSchema(
                common::SharedPtr<ContextT> ctx,
                CallbackT cb
            )
        {
            common::postAsyncTask(
                thread(),
                ctx,
                [ctx,cb{std::move(cb)},this](const common::SharedPtr<common::TaskContext>&)
                {
                    cb(std::move(ctx),m_client->checkSchema());
                }
            );
        }

        template <typename ContextT, typename CallbackT>
        void migrateSchema(
                common::SharedPtr<ContextT> ctx,
                CallbackT cb
            )
        {
            common::postAsyncTask(
                thread(),
                ctx,
                [ctx,cb{std::move(cb)},this](const common::SharedPtr<common::TaskContext>&)
                {
                    cb(std::move(ctx),m_client->migrateSchema());
                }
            );
        }

        template <typename ContextT, typename CallbackT>
        void addDatePartitions(
            common::SharedPtr<ContextT> ctx,
            CallbackT cb,
            std::vector<ModelInfo> models,
            common::Date to,
            common::Date from=common::Date::currentUtc()
            )
        {
            common::postAsyncTask(
                thread(),
                ctx,
                [ctx,cb{std::move(cb)},this,models{std::move(models)},to,from](const common::SharedPtr<common::TaskContext>&)
                {
                    cb(std::move(ctx),m_client->addDatePartitions(models,to,from));
                }
            );
        }

        template <typename ContextT, typename CallbackT>
        void listDatePartitions(
            common::SharedPtr<ContextT> ctx,
            CallbackT cb
            )
        {
            common::postAsyncTask(
                thread(),
                ctx,
                [ctx,cb{std::move(cb)},this](const common::SharedPtr<common::TaskContext>&)
                {
                    auto r=m_client->listDatePartitions();
                    if (r)
                    {
                        cb(std::move(ctx),r.takeError(),std::shared_ptr<Schema>{});
                    }
                    else
                    {
                        cb(std::move(ctx),Error{},r.takeValue());
                    }
                }
            );
        }

        template <typename ContextT, typename CallbackT>
        void deleteDatePartitions(
            common::SharedPtr<ContextT> ctx,
            CallbackT cb,
            const std::vector<ModelInfo>& models,
            common::Date to,
            common::Date from=common::Date::currentUtc()
            )
        {
            common::postAsyncTask(
                thread(),
                ctx,
                [ctx,cb{std::move(cb)},this,&models,&to,&from](const common::SharedPtr<common::TaskContext>&)
                {
                    cb(std::move(ctx),m_client->deleteDatePartitions(models,to,from));
                }
            );
        }

        template <typename ContextT, typename CallbackT>
        void deleteTopic(
            common::SharedPtr<ContextT> ctx,
            CallbackT cb,
            Topic topic
            )
        {
            common::postAsyncTask(
                thread(topic.topic()),
                ctx,
                [ctx,cb{std::move(cb)},this,&topic](const common::SharedPtr<common::TaskContext>&)
                {
                    cb(std::move(ctx),m_client->deleteTopic(topic));
                }
            );
        }

        template <typename ModelT, typename ContextT, typename CallbackT>
        void create(
            common::SharedPtr<ContextT> ctx,
            CallbackT cb,
            Topic topic,
            const std::shared_ptr<ModelT>& model,
            dataunit::Unit* object,
            Transaction* tx=nullptr
            )
        {
            common::postAsyncTask(
                thread(topic.topic()),
                ctx,
                [ctx,cb{std::move(cb)},this,&topic,&model,object,tx](const common::SharedPtr<common::TaskContext>&)
                {
                    cb(std::move(ctx),m_client->create(topic,model,object,tx));
                }
            );
        }

        template <typename ModelT, typename ContextT, typename CallbackT>
        void read(
            common::SharedPtr<ContextT> ctx,
            CallbackT cb,
            Topic topic,
            const std::shared_ptr<ModelT>& model,
            const ObjectId& id,
            Transaction* tx=nullptr,
            bool forUpdate=false,
            const TimePointFilter& tpFilter=TimePointFilter::getDefault()
            )
        {
            common::postAsyncTask(
                thread(topic.topic()),
                ctx,
                [ctx,cb{std::move(cb)},this,&topic,&model,&id,tx,forUpdate,&tpFilter](const common::SharedPtr<common::TaskContext>&)
                {
                    cb(std::move(ctx),m_client->read(topic,model,id,tx,forUpdate,tpFilter));
                }
            );
        }

        template <typename ModelT, typename ContextT, typename CallbackT>
        void read(
            common::SharedPtr<ContextT> ctx,
            CallbackT cb,
            Topic topic,
            const std::shared_ptr<ModelT>& model,
            const common::Date& date,
            Transaction* tx=nullptr,
            bool forUpdate=false,
            const TimePointFilter& tpFilter=TimePointFilter::getDefault()
            )
        {
            common::postAsyncTask(
                thread(topic.topic()),
                ctx,
                [ctx,cb{std::move(cb)},this,&topic,&model,&date,tx,forUpdate,&tpFilter](const common::SharedPtr<common::TaskContext>&)
                {
                    cb(std::move(ctx),m_client->read(topic,model,date,tx,forUpdate,tpFilter));
                }
            );
        }

        template <typename ModelT, typename ContextT, typename CallbackT>
        void update(
            common::SharedPtr<ContextT> ctx,
            CallbackT cb,
            Topic topic,
            const std::shared_ptr<ModelT>& model,
            const ObjectId& id,
            const update::Request& request,
            const common::Date& date,
            Transaction* tx=nullptr
            )
        {
            common::postAsyncTask(
                thread(topic.topic()),
                ctx,
                [ctx,cb{std::move(cb)},this,&topic,&model,&id,&request,&date,tx](const common::SharedPtr<common::TaskContext>&)
                {
                    cb(std::move(ctx),m_client->update(topic,model,id,request,date,tx));
                }
            );
        }

        template <typename ModelT, typename ContextT, typename CallbackT>
        void update(
            common::SharedPtr<ContextT> ctx,
            CallbackT cb,
            Topic topic,
            const std::shared_ptr<ModelT>& model,
            const ObjectId& id,
            const update::Request& request,
            Transaction* tx=nullptr
            )
        {
            common::postAsyncTask(
                thread(topic.topic()),
                ctx,
                [ctx,cb{std::move(cb)},this,&topic,&model,&id,&request,tx](const common::SharedPtr<common::TaskContext>&)
                {
                    cb(std::move(ctx),m_client->update(topic,model,id,request,tx));
                }
                );
        }

        template <typename ModelT, typename ContextT, typename CallbackT>
        void readUpdate(
            common::SharedPtr<ContextT> ctx,
            CallbackT cb,
            Topic topic,
            const std::shared_ptr<ModelT>& model,
            const ObjectId& id,
            const update::Request& request,
            const common::Date& date,
            update::ModifyReturn returnMode=update::ModifyReturn::After,
            Transaction* tx=nullptr,
            TimePointFilter tpFilter=TimePointFilter::getDefault()
            )
        {
            common::postAsyncTask(
                thread(topic.topic()),
                ctx,
                [ctx,cb{std::move(cb)},this,&topic,&model,&id,&request,&date,returnMode,tx,&tpFilter](const common::SharedPtr<common::TaskContext>&)
                {
                    cb(std::move(ctx),m_client->readUpdate(topic,model,id,request,date,returnMode,tx,tpFilter));
                }
                );
        }

        template <typename ModelT, typename ContextT, typename CallbackT>
        void readUpdate(
            common::SharedPtr<ContextT> ctx,
            CallbackT cb,
            Topic topic,
            const std::shared_ptr<ModelT>& model,
            const ObjectId& id,
            const update::Request& request,
            update::ModifyReturn returnMode=update::ModifyReturn::After,
            Transaction* tx=nullptr,
            TimePointFilter tpFilter=TimePointFilter::getDefault()
            )
        {
            common::postAsyncTask(
                thread(topic.topic()),
                ctx,
                [ctx,cb{std::move(cb)},this,&topic,&model,&id,&request,returnMode,tx,&tpFilter](const common::SharedPtr<common::TaskContext>&)
                {
                    cb(std::move(ctx),m_client->readUpdate(topic,model,id,request,returnMode,tx,tpFilter));
                }
            );
        }

        template <typename ModelT, typename ContextT, typename CallbackT>
        void deleteObject(
            common::SharedPtr<ContextT> ctx,
            CallbackT cb,
            Topic topic,
            const std::shared_ptr<ModelT>& model,
            const ObjectId& id,
            const common::Date& date,
            Transaction* tx=nullptr
            )
        {
            common::postAsyncTask(
                thread(topic.topic()),
                ctx,
                [ctx,cb{std::move(cb)},this,&topic,&model,&id,&date,tx](const common::SharedPtr<common::TaskContext>&)
                {
                    cb(std::move(ctx),m_client->deleteObject(topic,model,id,date,tx));
                }
            );
        }

        template <typename ModelT, typename ContextT, typename CallbackT>
        void deleteObject(
            common::SharedPtr<ContextT> ctx,
            CallbackT cb,
            Topic topic,
            const std::shared_ptr<ModelT>& model,
            const ObjectId& id,
            Transaction* tx=nullptr
            )
        {
            common::postAsyncTask(
                thread(topic.topic()),
                ctx,
                [ctx,cb{std::move(cb)},this,&topic,&model,&id,tx](const common::SharedPtr<common::TaskContext>&)
                {
                    cb(std::move(ctx),m_client->deleteObject(topic,model,id,tx));
                }
            );
        }

        template <typename ModelT, typename IndexT, typename ContextT, typename CallbackT>
        void find(
            common::SharedPtr<ContextT> ctx,
            CallbackT cb,
            const std::shared_ptr<ModelT>& model,
            const Query<IndexT>& query
            )
        {
            common::postAsyncTask(
                randomThread(),
                ctx,
                [ctx,cb{std::move(cb)},this,&model,&query](const common::SharedPtr<common::TaskContext>&)
                {
                    cb(std::move(ctx),m_client->find(model,query));
                }
            );
        }

        template <typename ModelT, typename IndexT, typename ContextT, typename CallbackT>
        void findOne(
            common::SharedPtr<ContextT> ctx,
            CallbackT cb,
            const std::shared_ptr<ModelT>& model,
            const Query<IndexT>& query
            )
        {
            common::postAsyncTask(
                randomThread(),
                ctx,
                [ctx,cb{std::move(cb)},this,&model,&query](const common::SharedPtr<common::TaskContext>&)
                {
                    cb(std::move(ctx),m_client->findOne(model,query));
                }
            );
        }

        template <typename ModelT, typename ContextT, typename CallbackT>
        void findAll(
            common::SharedPtr<ContextT> ctx,
            CallbackT cb,
            Topic topic,
            const std::shared_ptr<ModelT>& model,
            query::Order order=query::Asc
            )
        {
            common::postAsyncTask(
                thread(topic.topic()),
                ctx,
                [ctx,cb{std::move(cb)},this,&topic,&model,order](const common::SharedPtr<common::TaskContext>&)
                {
                    cb(std::move(ctx),m_client->findAll(topic,model,order));
                }
            );
        }

        template <typename ModelT, typename ContextT, typename CallbackT>
        void findAllPartitioned(
            common::SharedPtr<ContextT> ctx,
            CallbackT cb,
            Topic topic,
            const std::shared_ptr<ModelT>& model,
            query::Order order=query::Asc
            )
        {
            common::postAsyncTask(
                thread(topic.topic()),
                ctx,
                [ctx,cb{std::move(cb)},this,&topic,&model,order](const common::SharedPtr<common::TaskContext>&)
                {
                    cb(std::move(ctx),m_client->findAllPartitioned(topic,model,order));
                }
                );
        }

        template <typename ModelT, typename IndexT, typename ContextT, typename CallbackT>
        void findCb(
            common::SharedPtr<ContextT> ctx,
            CallbackT cb,
            const std::shared_ptr<ModelT>& model,
            const Query<IndexT>& query,
            FindCb findCb,
            Transaction* tx=nullptr,
            bool forUpdate=false
            )
        {
            common::postAsyncTask(
                randomThread(),
                ctx,
                [ctx,cb{std::move(cb)},this,&model,&query,findCb{std::move(findCb)},tx,forUpdate](const common::SharedPtr<common::TaskContext>&)
                {
                    cb(std::move(ctx),m_client->findCb(model,query,findCb,tx,forUpdate));
                }
            );
        }

        template <typename ModelT, typename IndexT, typename ContextT, typename CallbackT>
        void count(
            common::SharedPtr<ContextT> ctx,
            CallbackT cb,
            const std::shared_ptr<ModelT>& model,
            const Query<IndexT>& query
            )
        {
            common::postAsyncTask(
                randomThread(),
                ctx,
                [ctx,cb{std::move(cb)},this,&model,&query](const common::SharedPtr<common::TaskContext>&)
                {
                    cb(std::move(ctx),m_client->count(model,query));
                }
            );
        }

        template <typename ModelT, typename ContextT, typename CallbackT>
        void count(
            common::SharedPtr<ContextT> ctx,
            CallbackT cb,
            const std::shared_ptr<ModelT>& model
            )
        {
            common::postAsyncTask(
                randomThread(),
                ctx,
                [ctx,cb{std::move(cb)},this,&model](const common::SharedPtr<common::TaskContext>&)
                {
                    cb(std::move(ctx),m_client->count(model));
                }
            );
        }

        template <typename ModelT, typename ContextT, typename CallbackT>
        void count(
            common::SharedPtr<ContextT> ctx,
            CallbackT cb,
            const std::shared_ptr<ModelT>& model,
            Topic topic
            )
        {
            common::postAsyncTask(
                thread(topic.topic()),
                ctx,
                [ctx,cb{std::move(cb)},this,&topic,&model](const common::SharedPtr<common::TaskContext>&)
                {
                    cb(std::move(ctx),m_client->count(model,topic));
                }
            );
        }

        template <typename ModelT, typename ContextT, typename CallbackT>
        void count(
            common::SharedPtr<ContextT> ctx,
            CallbackT cb,
            const std::shared_ptr<ModelT>& model,
            const common::Date& date,
            Topic topic
            )
        {
            common::postAsyncTask(
                thread(topic.topic()),
                ctx,
                [ctx,cb{std::move(cb)},this,&topic,&model,&date](const common::SharedPtr<common::TaskContext>&)
                {
                    cb(std::move(ctx),m_client->count(model,date,topic));
                }
            );
        }

        template <typename ModelT, typename ContextT, typename CallbackT>
        void count(
            common::SharedPtr<ContextT> ctx,
            CallbackT cb,
            const std::shared_ptr<ModelT>& model,
            const common::Date& date
            )
        {
            common::postAsyncTask(
                randomThread(),
                ctx,
                [ctx,cb{std::move(cb)},this,&model,&date](const common::SharedPtr<common::TaskContext>&)
                {
                    cb(std::move(ctx),m_client->count(model,date));
                }
            );
        }

        template <typename ModelT, typename IndexT, typename ContextT, typename CallbackT>
        void deleteMany(
            common::SharedPtr<ContextT> ctx,
            CallbackT cb,
            const std::shared_ptr<ModelT>& model,
            const Query<IndexT>& query,
            Transaction* tx=nullptr
            )
        {
            common::postAsyncTask(
                randomThread(),
                ctx,
                [ctx,cb{std::move(cb)},this,&model,&query,tx](const common::SharedPtr<common::TaskContext>&)
                {
                    cb(std::move(ctx),m_client->deleteMany(model,query,tx));
                }
            );
        }

        template <typename ModelT, typename IndexT, typename ContextT, typename CallbackT>
        void deleteManyBulk(
            common::SharedPtr<ContextT> ctx,
            CallbackT cb,
            const std::shared_ptr<ModelT>& model,
            const Query<IndexT>& query,
            Transaction* tx=nullptr
            )
        {
            common::postAsyncTask(
                randomThread(),
                ctx,
                [ctx,cb{std::move(cb)},this,&model,&query,tx](const common::SharedPtr<common::TaskContext>&)
                {
                    cb(std::move(ctx),m_client->deleteManyBulk(model,query,tx));
                }
            );
        }

        template <typename ModelT, typename IndexT, typename ContextT, typename CallbackT>
        void updateMany(
            common::SharedPtr<ContextT> ctx,
            CallbackT cb,
            const std::shared_ptr<ModelT>& model,
            const Query<IndexT>& query,
            const update::Request& request,
            Transaction* tx=nullptr
            )
        {
            common::postAsyncTask(
                randomThread(),
                ctx,
                [ctx,cb{std::move(cb)},this,&model,&query,&request,tx](const common::SharedPtr<common::TaskContext>&)
                {
                    cb(std::move(ctx),m_client->updateMany(model,query,request,tx));
                }
            );
        }


        template <typename ModelT, typename IndexT, typename ContextT, typename CallbackT>
        void findUpdateCreate(
            common::SharedPtr<ContextT> ctx,
            CallbackT cb,
            const std::shared_ptr<ModelT>& model,
            const Query<IndexT>& query,
            const update::Request& request,
            const common::SharedPtr<dataunit::Unit>& object,
            update::ModifyReturn returnMode=update::ModifyReturn::After,
            Transaction* tx=nullptr
            )
        {
            common::postAsyncTask(
                randomThread(),
                ctx,
                [ctx,cb{std::move(cb)},this,&model,&query,&request,&object,returnMode,tx](const common::SharedPtr<common::TaskContext>&)
                {
                    cb(std::move(ctx),m_client->findUpdateCreate(model,query,request,object,returnMode,tx));
                }
                );
        }

        template <typename ModelT, typename IndexT, typename ContextT, typename CallbackT>
        void findUpdate(
            common::SharedPtr<ContextT> ctx,
            CallbackT cb,
            const std::shared_ptr<ModelT>& model,
            const Query<IndexT>& query,
            const update::Request& request,
            update::ModifyReturn returnMode=update::ModifyReturn::After,
            Transaction* tx=nullptr
            )
        {
            common::postAsyncTask(
                randomThread(),
                ctx,
                [ctx,cb{std::move(cb)},this,&model,&query,&request,returnMode,tx](const common::SharedPtr<common::TaskContext>&)
                {
                    cb(std::move(ctx),m_client->findUpdate(model,query,request,returnMode,tx));
                }
            );
        }

        template <typename ContextT, typename CallbackT>
        void transaction(
            common::SharedPtr<ContextT> ctx,
            CallbackT cb,
            TransactionFn fn
            )
        {
            common::postAsyncTask(
                thread(),
                ctx,
                [ctx,cb{std::move(cb)},this,fn{std::move(fn)}](const common::SharedPtr<common::TaskContext>&)
                {
                    cb(std::move(ctx),m_client->transaction(fn));
                }
            );
        }
};

HATN_DB_NAMESPACE_END

#endif // HATNDASYNCBCLIENT_H
