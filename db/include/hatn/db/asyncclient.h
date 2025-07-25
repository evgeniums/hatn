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

#include <memory>

#include <hatn/common/thread.h>
#include <hatn/common/taskcontext.h>

#include <hatn/db/db.h>
#include <hatn/db/client.h>

HATN_DB_NAMESPACE_BEGIN

class HATN_DB_EXPORT AsyncClient : public common::WithMappedThreads,
                                   public std::enable_shared_from_this<AsyncClient>
{
    public:

        AsyncClient(
                std::shared_ptr<Client> client,
                common::MappedThreadMode threadMode=common::MappedThreadMode::Caller,
                common::ThreadQWithTaskContext* defaultThread=common::ThreadQWithTaskContext::current()
            );

        AsyncClient(
                std::shared_ptr<Client> client,
                std::shared_ptr<common::MappedThreadQWithTaskContext> threads
            );

        std::shared_ptr<ClientEnvironment> cloneEnvironment()
        {
            return m_client->cloneEnvironment();
        }

        bool isOpen() const noexcept
        {            
            return m_client->isOpen();
        }

        auto client() const
        {
            return m_client;
        }

        template <typename ContextT, typename CallbackT>
        void openDb(
                common::SharedPtr<ContextT> ctx,
                CallbackT cb,
                const ClientConfig& config
            )
        {
            common::postAsyncTask(
                threads()->thread(),
                ctx,
                [ctx,this,self{shared_from_this()},&config](auto, auto cb)
                {
                    base::config_object::LogRecords records;
                    auto ec=m_client->openDb(config,records);
                    //! @todo Log error records
                    cb(std::move(ctx),std::move(ec));
                },
                std::move(cb)
            );
        }

        template <typename ContextT, typename CallbackT>
        void closeDb(
                common::SharedPtr<ContextT> ctx,
                CallbackT cb
            )
        {
            common::postAsyncTask(
                threads()->thread(),
                ctx,
                [ctx,this,self{shared_from_this()}](auto, auto cb)
                {
                    cb(std::move(ctx),m_client->closeDb());
                },
                std::move(cb)
            );
        }

        Error closeDbSync()
        {
            Error ec;
            std::ignore=threads()->thread()->execSync(
                [&ec,this,self{shared_from_this()}]()
                {
                    ec=m_client->closeDb();
                }
            );
            return ec;
        }

        template <typename ContextT, typename CallbackT>
        void createDb(
                common::SharedPtr<ContextT> ctx,
                CallbackT cb,
                const ClientConfig& config
            )
        {
            common::postAsyncTask(
                threads()->thread(),
                ctx,
                [ctx,this,self{shared_from_this()},&config](auto, auto cb)
                {
                    base::config_object::LogRecords records;
                    auto ec=m_client->createDb(config,records);
                    //! @todo Log error records
                    cb(std::move(ctx),std::move(ec));
                },
                std::move(cb)
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
                threads()->thread(),
                ctx,
                [ctx,this,self{shared_from_this()},&config](auto, auto cb)
                {
                    base::config_object::LogRecords records;
                    auto ec=m_client->destroyDb(config,records);
                    //! @todo Log error records
                    cb(std::move(ctx),std::move(ec));
                },
                std::move(cb)
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
                threads()->thread(),
                ctx,
                [ctx,this,self{shared_from_this()},schema{std::move(schema)}](auto, auto cb)
                {
                    cb(std::move(ctx),m_client->setSchema(std::move(schema)));
                },
                std::move(cb)
            );
        }

        template <typename ContextT, typename CallbackT>
        void schema(
                common::SharedPtr<ContextT> ctx,
                CallbackT cb
            )
        {
            common::postAsyncTask(
                threads()->thread(),
                ctx,
                [ctx,this,self{shared_from_this()}](auto, auto cb)
                {
                    cb(std::move(ctx),m_client->schema());
                },
                std::move(cb)
            );
        }

        template <typename ContextT, typename CallbackT>
        void checkSchema(
                common::SharedPtr<ContextT> ctx,
                CallbackT cb
            )
        {
            common::postAsyncTask(
                threads()->thread(),
                ctx,
                [ctx,this,self{shared_from_this()}](auto, auto cb)
                {
                    cb(std::move(ctx),m_client->checkSchema());
                },
                std::move(cb)
            );
        }

        template <typename ContextT, typename CallbackT>
        void migrateSchema(
                common::SharedPtr<ContextT> ctx,
                CallbackT cb
            )
        {
            common::postAsyncTask(
                threads()->thread(),
                ctx,
                [ctx,this,self{shared_from_this()}](auto, auto cb)
                {
                    cb(std::move(ctx),m_client->migrateSchema());
                },
                std::move(cb)
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
                threads()->thread(),
                ctx,
                [ctx,this,self{shared_from_this()},models{std::move(models)},to,from](auto, auto cb)
                {
                    cb(std::move(ctx),m_client->addDatePartitions(models,to,from));
                },
                std::move(cb)
            );
        }

        template <typename ContextT, typename CallbackT>
        void listDatePartitions(
            common::SharedPtr<ContextT> ctx,
            CallbackT cb
            )
        {
            common::postAsyncTask(
                threads()->thread(),
                ctx,
                [ctx,this,self{shared_from_this()}](auto, auto cb)
                {
                    cb(std::move(ctx),m_client->listDatePartitions());
                },
                std::move(cb)
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
                threads()->thread(),
                ctx,
                [ctx,this,self{shared_from_this()},&models,&to,&from](auto, auto cb)
                {
                    cb(std::move(ctx),m_client->deleteDatePartitions(models,to,from));
                },
                std::move(cb)
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
                threads()->thread(topic.topic()),
                ctx,
                [ctx,this,self{shared_from_this()},topic](auto, auto cb)
                {
                    cb(std::move(ctx),m_client->deleteTopic(topic));
                },
                std::move(cb)
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
                threads()->thread(topic.topic()),
                ctx,
                [ctx,this,self{shared_from_this()},topic,&model,object,tx](auto, auto cb)
                {
                    cb(std::move(ctx),m_client->create(topic,model,object,tx));
                },
                std::move(cb)
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
                threads()->thread(topic.topic()),
                ctx,
                [ctx,this,self{shared_from_this()},topic,&model,id,tx,forUpdate,&tpFilter](auto, auto cb)
                {
                    cb(std::move(ctx),m_client->read(topic,model,id,tx,forUpdate,tpFilter));
                },
                std::move(cb)
            );
        }

        template <typename ModelT, typename ContextT, typename CallbackT>
        void read(
            common::SharedPtr<ContextT> ctx,
            CallbackT cb,
            Topic topic,
            const std::shared_ptr<ModelT>& model,
            const ObjectId& id,
            common::Date date,
            Transaction* tx=nullptr,
            bool forUpdate=false,
            const TimePointFilter& tpFilter=TimePointFilter::getDefault()
            )
        {
            common::postAsyncTask(
                threads()->thread(topic.topic()),
                ctx,
                [ctx,this,self{shared_from_this()},topic,&model,id,date,tx,forUpdate,&tpFilter](auto, auto cb)
                {
                    cb(std::move(ctx),m_client->read(topic,model,id,date,tx,forUpdate,tpFilter));
                },
                std::move(cb)
            );
        }

        template <typename ModelT, typename ContextT, typename CallbackT>
        void update(
            common::SharedPtr<ContextT> ctx,
            CallbackT cb,
            Topic topic,
            const std::shared_ptr<ModelT>& model,
            const ObjectId& id,
            common::SharedPtr<update::Request> request,
            common::Date date,
            Transaction* tx=nullptr
            )
        {
            common::postAsyncTask(
                threads()->thread(topic.topic()),
                ctx,
                [ctx,this,self{shared_from_this()},topic,&model,id,request{std::move(request)},date,tx](auto, auto cb)
                {
                    cb(std::move(ctx),m_client->update(topic,model,id,*request,date,tx));
                },
                std::move(cb)
            );
        }

        template <typename ModelT, typename ContextT, typename CallbackT>
        void update(
            common::SharedPtr<ContextT> ctx,
            CallbackT cb,
            Topic topic,
            const std::shared_ptr<ModelT>& model,
            const ObjectId& id,
            common::SharedPtr<update::Request> request,
            Transaction* tx=nullptr
            )
        {
            common::postAsyncTask(
                threads()->thread(topic.topic()),
                ctx,
                [ctx,this,self{shared_from_this()},topic,&model,id,request{std::move(request)},tx](auto, auto cb)
                {
                    cb(std::move(ctx),m_client->update(topic,model,id,*request,tx));
                },
                std::move(cb)
            );
        }

        template <typename ModelT, typename ContextT, typename CallbackT>
        void readUpdate(
            common::SharedPtr<ContextT> ctx,
            CallbackT cb,
            Topic topic,
            const std::shared_ptr<ModelT>& model,
            const ObjectId& id,
            common::SharedPtr<update::Request> request,
            common::Date date,
            update::ModifyReturn returnMode=update::ModifyReturn::After,
            Transaction* tx=nullptr,
            TimePointFilter tpFilter=TimePointFilter::getDefault()
            )
        {            
            common::postAsyncTask(
                threads()->thread(topic.topic()),
                ctx,
                [ctx,this,self{shared_from_this()},topic,&model,id,request{std::move(request)},date,returnMode,tx,&tpFilter](auto, auto cb)
                {
                    cb(std::move(ctx),m_client->readUpdate(topic,model,id,*request,date,returnMode,tx,tpFilter));
                },
                std::move(cb)
            );
        }

        template <typename ModelT, typename ContextT, typename CallbackT>
        void readUpdate(
            common::SharedPtr<ContextT> ctx,
            CallbackT cb,
            Topic topic,
            const std::shared_ptr<ModelT>& model,
            const ObjectId& id,
            common::SharedPtr<update::Request> request,
            update::ModifyReturn returnMode=update::ModifyReturn::After,
            Transaction* tx=nullptr,
            TimePointFilter tpFilter=TimePointFilter::getDefault()
            )
        {            
            common::postAsyncTask(
                threads()->thread(topic.topic()),
                ctx,
                [ctx,this,self{shared_from_this()},topic,&model,id,request{std::move(request)},returnMode,tx,&tpFilter](auto, auto cb)
                {
                    cb(std::move(ctx),m_client->readUpdate(topic,model,id,*request,returnMode,tx,tpFilter));
                },
                std::move(cb)
            );
        }

        template <typename ModelT, typename ContextT, typename CallbackT>
        void deleteObject(
            common::SharedPtr<ContextT> ctx,
            CallbackT cb,
            Topic topic,
            const std::shared_ptr<ModelT>& model,
            const ObjectId& id,
            common::Date date,
            Transaction* tx=nullptr
            )
        {
            common::postAsyncTask(
                threads()->thread(topic.topic()),
                ctx,
                [ctx,this,self{shared_from_this()},topic,&model,id,date,tx](auto, auto cb)
                {
                    cb(std::move(ctx),m_client->deleteObject(topic,model,id,date,tx));
                },
                std::move(cb)
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
                threads()->thread(topic.topic()),
                ctx,
                [ctx,this,self{shared_from_this()},topic,&model,id,tx](auto, auto cb)
                {
                    cb(std::move(ctx),m_client->deleteObject(topic,model,id,tx));
                },
                std::move(cb)
            );
        }

        template <typename ModelT, typename ContextT, typename CallbackT, typename QueryT>
        void find(
            common::SharedPtr<ContextT> ctx,
            CallbackT cb,
            const std::shared_ptr<ModelT>& model,
            QueryT query,
            Topic topic={}
            )
        {
            common::postAsyncTask(
                topicOrRandomThread(topic),
                ctx,
                [ctx,this,self{shared_from_this()},&model,query{std::move(query)}](auto, auto cb)
                {
                    auto r=m_client->find(model,query());
                    cb(std::move(ctx),std::move(r));
                },
                std::move(cb)
            );
        }

        template <typename ModelT, typename ContextT, typename CallbackT, typename QueryT>
        void findOne(
            common::SharedPtr<ContextT> ctx,
            CallbackT cb,
            const std::shared_ptr<ModelT>& model,
            QueryT query,
            Topic topic={}
            )
        {
            common::postAsyncTask(
                topicOrRandomThread(topic),
                ctx,
                [ctx,this,self{shared_from_this()},&model,query{std::move(query)}](auto, auto cb)
                {
                    cb(std::move(ctx),m_client->findOne(model,query()));
                },
                std::move(cb)
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
                threads()->thread(topic.topic()),
                ctx,
                [ctx,this,self{shared_from_this()},topic,&model,order](auto, auto cb)
                {
                    cb(std::move(ctx),m_client->findAll(topic,model,order));
                },
                std::move(cb)
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
                threads()->thread(topic.topic()),
                ctx,
                [ctx,this,self{shared_from_this()},topic,&model,order](auto, auto cb)
                {
                    cb(std::move(ctx),m_client->findAllPartitioned(topic,model,order));
                },
                std::move(cb)
            );
        }

        template <typename ModelT, typename ContextT, typename CallbackT, typename QueryT>
        void count(
            common::SharedPtr<ContextT> ctx,
            CallbackT cb,
            const std::shared_ptr<ModelT>& model,
            QueryT query,
            Topic topic={}
            )
        {
            common::postAsyncTask(
                topicOrRandomThread(topic),
                ctx,
                [ctx,this,self{shared_from_this()},&model,query{std::move(query)}](auto, auto cb)
                {
                    cb(std::move(ctx),m_client->count(model,query()));
                },
                std::move(cb)
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
                threads()->randomThread(),
                ctx,
                [ctx,this,self{shared_from_this()},&model](auto, auto cb)
                {
                    cb(std::move(ctx),m_client->count(model));
                },
                std::move(cb)
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
                threads()->thread(topic.topic()),
                ctx,
                [ctx,this,self{shared_from_this()},topic,&model](auto, auto cb)
                {
                    cb(std::move(ctx),m_client->count(model,topic));
                },
                std::move(cb)
            );
        }

        template <typename ModelT, typename ContextT, typename CallbackT>
        void count(
            common::SharedPtr<ContextT> ctx,
            CallbackT cb,
            const std::shared_ptr<ModelT>& model,
            common::Date date,
            Topic topic
            )
        {
            common::postAsyncTask(
                threads()->thread(topic.topic()),
                ctx,
                [ctx,this,self{shared_from_this()},topic,&model,date](auto, auto cb)
                {
                    cb(std::move(ctx),m_client->count(model,date,topic));
                },
                std::move(cb)
            );
        }

        template <typename ModelT, typename ContextT, typename CallbackT>
        void count(
            common::SharedPtr<ContextT> ctx,
            CallbackT cb,
            const std::shared_ptr<ModelT>& model,
            common::Date date
            )
        {
            common::postAsyncTask(
                threads()->randomThread(),
                ctx,
                [ctx,this,self{shared_from_this()},&model,date](auto, auto cb)
                {
                    cb(std::move(ctx),m_client->count(model,date));
                },
                std::move(cb)
            );
        }

        template <typename ModelT, typename ContextT, typename CallbackT, typename QueryT>
        void deleteMany(
            common::SharedPtr<ContextT> ctx,
            CallbackT cb,
            const std::shared_ptr<ModelT>& model,
            QueryT query,
            Transaction* tx=nullptr,
            Topic topic={}
            )
        {
            common::postAsyncTask(
                topicOrRandomThread(topic),
                ctx,
                [ctx,this,self{shared_from_this()},&model,query{std::move(query)},tx](auto, auto cb)
                {
                    cb(std::move(ctx),m_client->deleteMany(model,query(),tx));
                },
                std::move(cb)
            );
        }

        template <typename ModelT, typename ContextT, typename CallbackT, typename QueryT>
        void deleteManyBulk(
            common::SharedPtr<ContextT> ctx,
            CallbackT cb,
            const std::shared_ptr<ModelT>& model,
            QueryT query,
            Transaction* tx=nullptr,
            Topic topic={}
            )
        {
            common::postAsyncTask(
                topicOrRandomThread(topic),
                ctx,
                [ctx,this,self{shared_from_this()},&model,query{std::move(query)},tx](auto, auto cb)
                {
                    cb(std::move(ctx),m_client->deleteManyBulk(model,query(),tx));
                },
                std::move(cb)
            );
        }

        template <typename ModelT, typename ContextT, typename CallbackT, typename QueryT>
        void updateMany(
            common::SharedPtr<ContextT> ctx,
            CallbackT cb,
            const std::shared_ptr<ModelT>& model,
            QueryT query,
            common::SharedPtr<update::Request> request,
            Transaction* tx=nullptr,
            Topic topic={}
            )
        {
            common::postAsyncTask(
                topicOrRandomThread(topic),
                ctx,
                [ctx,this,self{shared_from_this()},&model,query{std::move(query)},request{std::move(request)},tx](auto, auto cb)
                {
                    cb(std::move(ctx),m_client->updateMany(model,query(),*request,tx));
                },
                std::move(cb)
            );
        }


        template <typename ModelT, typename ContextT, typename CallbackT, typename QueryT>
        void findUpdateCreate(
            common::SharedPtr<ContextT> ctx,
            CallbackT cb,
            const std::shared_ptr<ModelT>& model,
            QueryT query,
            common::SharedPtr<update::Request> request,
            const common::SharedPtr<dataunit::Unit>& object,
            update::ModifyReturn returnMode=update::ModifyReturn::After,
            Transaction* tx=nullptr,
            Topic topic={}
            )
        {
            common::postAsyncTask(
                topicOrRandomThread(topic),
                ctx,
                [ctx,this,self{shared_from_this()},&model,query{std::move(query)},request{std::move(request)},&object,returnMode,tx](auto, auto cb)
                {
                    cb(std::move(ctx),m_client->findUpdateCreate(model,query(),*request,object,returnMode,tx));
                },
                std::move(cb)
            );
        }

        template <typename ModelT, typename ContextT, typename CallbackT, typename QueryT>
        void findUpdate(
            common::SharedPtr<ContextT> ctx,
            CallbackT cb,
            const std::shared_ptr<ModelT>& model,
            QueryT query,
            common::SharedPtr<update::Request> request,
            update::ModifyReturn returnMode=update::ModifyReturn::After,
            Transaction* tx=nullptr,
            Topic topic={}
            )
        {
            common::postAsyncTask(
                topicOrRandomThread(topic),
                ctx,
                [ctx,this,self{shared_from_this()},&model,query{std::move(query)},request{std::move(request)},returnMode,tx](auto, auto cb)
                {
                    cb(std::move(ctx),m_client->findUpdate(model,query(),*request,returnMode,tx));
                },
                std::move(cb)
            );
        }

        template <typename ModelT,typename ContextT, typename CallbackT>
        Result<std::pmr::set<TopicHolder>>
        listModelTopics(
            common::SharedPtr<ContextT> ctx,
            CallbackT cb,
            const std::shared_ptr<ModelT>& model,
            common::Date partitionDate,
            bool onlyDefaultPartition=true
            )
        {
            common::postAsyncTask(
                threads()->randomThread(),
                ctx,
                [ctx,this,self{shared_from_this()},&model,partitionDate,onlyDefaultPartition](auto, auto cb)
                {
                    cb(std::move(ctx),m_client->listModelTopics(model,partitionDate,onlyDefaultPartition));
                },
                std::move(cb)
            );
        }

        template <typename ModelT,typename ContextT, typename CallbackT>
        Result<std::pmr::set<TopicHolder>>
        listModelTopics(
            common::SharedPtr<ContextT> ctx,
            CallbackT cb,
            const std::shared_ptr<ModelT>& model,
            common::DateRange partitionDateRange={},
            bool onlyDefaultPartition=true
            )
        {
            common::postAsyncTask(
                threads()->randomThread(),
                ctx,
                [ctx,this,self{shared_from_this()},&model,partitionDateRange,onlyDefaultPartition](auto, auto cb)
                {
                    cb(std::move(ctx),m_client->listModelTopics(model,partitionDateRange,onlyDefaultPartition));
                },
                std::move(cb)
            );
        }

        template <typename ContextT, typename CallbackT>
        void transaction(
            common::SharedPtr<ContextT> ctx,
            CallbackT cb,
            TransactionFn fn,
            Topic topic={}
            )
        {
            common::postAsyncTask(
                topicOrRandomThread(topic),
                ctx,
                [ctx,this,self{shared_from_this()},fn{std::move(fn)}](auto, auto cb)
                {
                    cb(std::move(ctx),m_client->transaction(fn));
                },
                std::move(cb)
            );
        }

    private:

        common::ThreadQWithTaskContext* topicOrRandomThread(Topic topic={}) const noexcept
        {
            if (topic.topic().empty())
            {
                return threads()->randomThread();
            }
            return threads()->thread(topic.topic());
        }

        std::shared_ptr<Client> m_client;
};

class WithAsyncClient
{
    public:

        WithAsyncClient(std::shared_ptr<AsyncClient> db={}) : m_db(std::move(db))
        {}

        void setDbClient(std::shared_ptr<AsyncClient> db) noexcept
        {
            m_db=std::move(db);
        }

        const std::shared_ptr<AsyncClient>& dbClient() const
        {
            Assert(m_db,"async database client must be set");
            return m_db;
        }

        Error close()
        {
            if (m_db)
            {
                return m_db->closeDbSync();
            }
            return OK;
        }

        void reset()
        {
            m_db.reset();
        }

        Error setSchema(std::shared_ptr<Schema> schema)
        {
            return m_db->client()->setSchema(std::move(schema));
        }

    private:

        std::shared_ptr<AsyncClient> m_db;
};

class SingleAsyncClient : public WithAsyncClient
{
    public:

        using WithAsyncClient::WithAsyncClient;

        const std::shared_ptr<AsyncClient>& dbClient(const Topic&) const noexcept
        {
            return WithAsyncClient::dbClient();
        }

        void registerDbClient(const Topic&, std::shared_ptr<AsyncClient> client)
        {
            setDbClient(client);
        }
};

class MultipleAsyncClients : public WithAsyncClient
{
    public:

        MultipleAsyncClients(std::shared_ptr<AsyncClient> db={})
            : WithAsyncClient(std::move(db))
        {}

        using WithAsyncClient::dbClient;

        const std::shared_ptr<AsyncClient>& dbClient(const Topic& topic) const noexcept
        {
            auto it=m_clients.find(topic.topic());
            if (it==m_clients.end())
            {
                return WithAsyncClient::dbClient();
            }
            return it->second;
        }

        void registerDbClient(std::string topic, std::shared_ptr<AsyncClient> client)
        {
            if (topic.empty())
            {
                setDbClient(std::move(client));
            }
            else
            {
                m_clients[std::move(topic)]=std::move(client);
            }
        }

        Error close()
        {
            Error ec;
            for (auto&& it: m_clients)
            {
                auto ec1=it.second->closeDbSync();
                if (!ec && ec1)
                {
                    ec=ec1;
                }
            }
            auto ec1=WithAsyncClient::close();
            if (!ec && ec1)
            {
                ec=ec1;
            }
            return ec;
        }

        void reset()
        {
            m_clients.clear();
            WithAsyncClient::reset();
        }

        Error setSchema(std::shared_ptr<Schema> schema)
        {
            for (auto&& it:m_clients)
            {
                auto ec=it.second->client()->setSchema(schema);
                HATN_CHECK_EC(ec)
            }

            return WithAsyncClient::setSchema(std::move(schema));
        }

    private:

        common::FlatMap<std::string,std::shared_ptr<AsyncClient>,std::less<>> m_clients;
};

#ifndef HATN_DB_MAPPED_ASYNC_CLIENTS
using MappedAsyncClients=MultipleAsyncClients;
#else
using MappedAsyncClients==HATN_DB_MAPPED_ASYNC_CLIENTS;
#endif

using AsyncDb=MappedAsyncClients;

HATN_DB_NAMESPACE_END

#endif // HATNDASYNCBCLIENT_H

//! @todo Test async client
