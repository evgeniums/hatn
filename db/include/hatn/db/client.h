/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/client.h
  *
  * Contains declaration of base database client.
  *
  */

/****************************************************************************/

#ifndef HATNDBCLIENT_H
#define HATNDBCLIENT_H

#include <vector>
#include <set>

#include <hatn/common/error.h>
#include <hatn/common/runonscopeexit.h>
#include <hatn/common/objectid.h>

#include <hatn/logcontext/context.h>
#include <hatn/logcontext/contextlogger.h>

#include <hatn/base/configtree.h>
#include <hatn/base/configobject.h>

#include <hatn/db/db.h>
#include <hatn/db/dberror.h>
#include <hatn/db/model.h>
#include <hatn/db/schema.h>
#include <hatn/db/topic.h>
#include <hatn/db/indexquery.h>
#include <hatn/db/update.h>
#include <hatn/db/transaction.h>

HATN_DB_NAMESPACE_BEGIN

struct ClientConfig
{
    const base::ConfigTree& main;
    const base::ConfigTree& opt;

    base::ConfigTreePath mainPath;
    base::ConfigTreePath optPath;
};

class HATN_DB_EXPORT Client : public common::WithID
{
    public:

        Client(common::STR_ID_TYPE id=common::STR_ID_TYPE());
        virtual ~Client();

        Client(const Client&)=delete;
        Client(Client&&)=default;
        Client& operator=(const Client&)=delete;
        Client& operator=(Client&&)=default;

        //! @todo Implement Delete topic

        bool isOpen() const noexcept
        {
            return m_opened;
        }

        Error openDb(const ClientConfig& config, base::config_object::LogRecords& records)
        {
            HATN_CTX_SCOPE("dbopendb")

            if (m_opened)
            {
                HATN_CTX_SCOPE_LOCK()
                return dbError(DbError::DB_ALREADY_OPEN);
            }

            Error ec;
            auto onExit=[this,&ec](){
                if (!ec)
                {
                    m_opened=true;
                }
            };
            auto scopeGuard=HATN_COMMON_NAMESPACE::makeScopeGuard(std::move(onExit));\
            std::ignore=scopeGuard;
            doOpenDb(config,ec,records);
            return ec;
        }

        Error closeDb()
        {
            HATN_CTX_SCOPE("dbclosedb")
            if (m_opened)
            {
                Error ec;
                HATN_SCOPE_GUARD([this](){m_opened=false;})
                doCloseDb(ec);
                return ec;
            }
            return OK;
        }

        Error createDb(const ClientConfig& config, base::config_object::LogRecords& records)
        {
            HATN_CTX_SCOPE("dbcreatedb")
            return doCreateDb(config,records);
        }

        Error destroyDb(const ClientConfig& config, base::config_object::LogRecords& records)
        {
            HATN_CTX_SCOPE("dbdestroydb")
            return doDestroyDb(config,records);
        }

        Error setSchema(std::shared_ptr<Schema> schema)
        {
            HATN_CTX_SCOPE("dbsetschema")
            if (m_opened)
            {
                return doSetSchema(std::move(schema));
            }
            HATN_CTX_SCOPE_LOCK()
            return dbError(DbError::DB_NOT_OPEN);
        }

        Result<std::shared_ptr<Schema>> schema() const
        {
            HATN_CTX_SCOPE("dbschema")
            if (m_opened)
            {
                return doGetSchema();
            }
            HATN_CTX_SCOPE_LOCK()
            return dbError(DbError::DB_NOT_OPEN);
        }

        Error checkSchema()
        {
            HATN_CTX_SCOPE("dbcheckschema")
            if (m_opened)
            {
                return doCheckSchema();
            }
            HATN_CTX_SCOPE_LOCK()
            return dbError(DbError::DB_NOT_OPEN);
        }

        Error migrateSchema()
        {
            HATN_CTX_SCOPE("dbmigrateschema")
            if (m_opened)
            {
                return doMigrateSchema();
            }
            HATN_CTX_SCOPE_LOCK()
            return dbError(DbError::DB_NOT_OPEN);
        }

        Error addDatePartitions(const std::vector<ModelInfo>& models, const common::Date& to, const common::Date& from=common::Date{})
        {
            HATN_CTX_SCOPE("dbadddatepartitions")
            if (m_opened)
            {
                return doAddDatePartitions(models,datePartitionRanges(models,to,from));
            }
            HATN_CTX_SCOPE_LOCK()
            return dbError(DbError::DB_NOT_OPEN);
        }

        Error deleteDatePartitions(const std::vector<ModelInfo>& models, const common::Date& to, const common::Date& from=common::Date{})
        {
            HATN_CTX_SCOPE("dbdeletedatepartitions")
            if (m_opened)
            {
                return doDeleteDatePartitions(models,datePartitionRanges(models,to,from));
            }
            HATN_CTX_SCOPE_LOCK()
            return dbError(DbError::DB_NOT_OPEN);
        }

        static std::set<common::DateRange> datePartitionRanges(const std::vector<ModelInfo>& models, const common::Date& to, const common::Date& from=common::Date{});

        template <typename ModelT>
        Error create(const Topic& topic,
                     const std::shared_ptr<ModelT>& model,
                     dataunit::Unit* object,
                     Transaction* tx=nullptr
                     )
        {
            HATN_CTX_SCOPE("dbcreate")
            if (m_opened)
            {
                return doCreate(topic,*model->info,object,tx);
            }
            HATN_CTX_SCOPE_LOCK()
            return dbError(DbError::DB_NOT_OPEN);
        }

        template <typename ModelT>
        Result<typename ModelT::SharedPtr> read(const Topic& topic,
                                                const std::shared_ptr<ModelT>& model,
                                                const ObjectId& id,
                                                Transaction* tx=nullptr,
                                                bool forUpdate=false,
                                                const TimePointFilter& tpFilter=TimePointFilter{})
        {
            HATN_CTX_SCOPE("dbread")
            if (m_opened)
            {
                return afterRead(model,doRead(topic,*model->info,id,tx,forUpdate),tpFilter);
            }

            HATN_CTX_SCOPE_LOCK()
            return dbError(DbError::DB_NOT_OPEN);
        }

        template <typename ModelT>
        Result<typename ModelT::SharedPtr> read(const Topic& topic,
                                                const std::shared_ptr<ModelT>& model,
                                                const ObjectId& id,
                                                const common::Date& date,
                                                Transaction* tx=nullptr,
                                                bool forUpdate=false,
                                                const TimePointFilter& tpFilter=TimePointFilter{})
        {
            HATN_CTX_SCOPE("dbreaddate")
            if (m_opened)
            {
                afterRead(model,doRead(topic,*model->info,id,date,tx,forUpdate),tpFilter);
            }

            HATN_CTX_SCOPE_LOCK()
            return dbError(DbError::DB_NOT_OPEN);
        }

        template <typename ModelT>
        Error update(const Topic& topic,
                       const std::shared_ptr<ModelT>& model,
                       const update::Request& request,
                       const ObjectId& id,
                       const common::Date& date,
                       Transaction* tx=nullptr)
        {
            HATN_CTX_SCOPE("dbupdatedate")
            if (m_opened)
            {
                return doUpdateObject(topic,*model->info,request,id,date,tx);
            }

            HATN_CTX_SCOPE_LOCK()
            return dbError(DbError::DB_NOT_OPEN);
        }

        template <typename ModelT>
        Error update(const Topic& topic,
                     const std::shared_ptr<ModelT>& model,
                     const update::Request& request,
                     const ObjectId& id,
                     Transaction* tx=nullptr)
        {
            HATN_CTX_SCOPE("dbupdate")
            if (m_opened)
            {
                return doUpdateObject(topic,*model->info,request,id,tx);
            }

            HATN_CTX_SCOPE_LOCK()
            return dbError(DbError::DB_NOT_OPEN);
        }

        template <typename ModelT>
        Result<typename ModelT::SharedPtr> readUpdate(const Topic& topic,
                           const std::shared_ptr<ModelT>& model,
                           const update::Request& request,
                           const ObjectId& id,
                           const common::Date& date,                           
                           update::ModifyReturn returnType=update::ModifyReturn::After,
                           Transaction* tx=nullptr,
                           const TimePointFilter& tpFilter=TimePointFilter{}
                    )
        {
            HATN_CTX_SCOPE("dbreadupdatedate")
            if (m_opened)
            {
                return afterRead(model,doReadUpdate(topic,*model->info,id,request,date,returnType,tx),tpFilter);
            }

            HATN_CTX_SCOPE_LOCK()
            return dbError(DbError::DB_NOT_OPEN);
        }

        template <typename ModelT>
        Result<typename ModelT::SharedPtr> readUpdate(const Topic& topic,
                                                      const std::shared_ptr<ModelT>& model,
                                                      const ObjectId& id,
                                                      const update::Request& request,
                                                      update::ModifyReturn returnType=update::ModifyReturn::After,
                                                      Transaction* tx=nullptr,
                                                      const TimePointFilter& tpFilter=TimePointFilter{}
                                                      )
        {
            HATN_CTX_SCOPE("dbreadupdate")
            if (m_opened)
            {
                return afterRead(model,doReadUpdate(topic,*model->info,id,request,returnType,tx),tpFilter);
            }

            HATN_CTX_SCOPE_LOCK()
            return dbError(DbError::DB_NOT_OPEN);
        }


        template <typename ModelT>
        Error deleteObject(const Topic& topic,
                            const std::shared_ptr<ModelT>& model,
                            const ObjectId& id,
                            const common::Date& date,
                            Transaction* tx=nullptr)
        {
            HATN_CTX_SCOPE("dbdelete")
            if (m_opened)
            {
                return doDeleteObject(topic,*model->info,id,date,tx);
            }

            HATN_CTX_SCOPE_LOCK()
            return dbError(DbError::DB_NOT_OPEN);
        }

        template <typename ModelT>
        Error deleteObject(const Topic& topic,
                            const std::shared_ptr<ModelT>& model,
                            const ObjectId& id,
                            Transaction* tx=nullptr)
        {
            HATN_CTX_SCOPE("dbdelete")
            if (m_opened)
            {
                return doDeleteObject(topic,*model->info,id,tx);
            }

            HATN_CTX_SCOPE_LOCK()
            return dbError(DbError::DB_NOT_OPEN);
        }

        //! @todo Add count()
        //! @todo Return topic in find results

        template <typename ModelT, typename IndexT>
        Result<HATN_COMMON_NAMESPACE::pmr::vector<UnitWrapper>> find(
            const std::shared_ptr<ModelT>& model,
            const Query<IndexT>& query,
            bool single=false
        )
        {
            HATN_CTX_SCOPE("dbfind")
            if (m_opened)
            {
                ModelIndexQuery q{query,model->model.indexId(query.indexT())};

                auto r=doFind(*model->info,q,single);
                HATN_CHECK_RESULT(r)
                if (!query.hasFilterTimePoints())
                {
                    return r;
                }
                using unitT=typename ModelT::Type;
                HATN_COMMON_NAMESPACE::pmr::vector<UnitWrapper> v{r.value().get_allocator()};
                v.reserve(r.value().size());
                for (auto&& it:r.value())
                {
                    if (query.filterTimePoint(*(it.template unit<unitT>())))
                    {
                        v.push_back(it);
                    }
                }
                return v;
            }

            HATN_CTX_SCOPE_LOCK()
            return dbError(DbError::DB_NOT_OPEN);
        }

        //! @todo Add find with callback per each found object

        template <typename ModelT, typename IndexT>
        Result<typename ModelT::SharedPtr> findOne(
            const std::shared_ptr<ModelT>& model,
            const Query<IndexT>& query
            )
        {
            HATN_CTX_SCOPE("dbfindone")
            auto r=find(model,query,true);
            HATN_CHECK_RESULT(r)
            if (r->empty())
            {
                return typename ModelT::SharedPtr{};
            }
            return r->at(0).template managedUnit<typename ModelT::ManagedType>()->sharedFromThis();
        }

        template <typename ModelT, typename QueryT>
        Result<size_t> count(
            const std::shared_ptr<ModelT>& model,
            const QueryT& query
            )
        {
            HATN_CTX_SCOPE("dbcount")
            if (m_opened)
            {
                ModelIndexQuery q{query,model->model.indexId(query.indexT())};
                return doCount(*model->info,q);
            }

            HATN_CTX_SCOPE_LOCK()
            return dbError(DbError::DB_NOT_OPEN);
        }

        template <typename ModelT, typename QueryT>
        Result<size_t> deleteMany(
                const std::shared_ptr<ModelT>& model,
                const QueryT& query,
                Transaction* tx=nullptr
            )
        {
            HATN_CTX_SCOPE("dbdeletemany")
            if (m_opened)
            {
                ModelIndexQuery q{query,model->model.indexId(query.indexT())};
                return doDeleteMany(*model->info,q,tx);
            }

            HATN_CTX_SCOPE_LOCK()
            return dbError(DbError::DB_NOT_OPEN);
        }

        template <typename ModelT, typename QueryT>
        Result<size_t> updateMany(
                const std::shared_ptr<ModelT>& model,
                const QueryT& query,
                const update::Request& request,
                Transaction* tx=nullptr
            )
        {
            HATN_CTX_SCOPE("dbupdatemany")
            if (m_opened)
            {
                ModelIndexQuery q{query,model->model.indexId(query.indexT())};
                return doUpdateMany(*model->info,q,request,tx);
            }

            HATN_CTX_SCOPE_LOCK()
            return dbError(DbError::DB_NOT_OPEN);
        }

        template <typename ModelT, typename QueryT>
        Result<typename ModelT::SharedPtr> findUpdateCreate(
                                                      const std::shared_ptr<ModelT>& model,
                                                      const QueryT& query,
                                                      const update::Request& request,
                                                      const HATN_COMMON_NAMESPACE::SharedPtr<dataunit::Unit>& object,
                                                      update::ModifyReturn returnType=update::ModifyReturn::After,
                                                      Transaction* tx=nullptr)
        {
            HATN_CTX_SCOPE("dbfindupdatecreate")
            if (m_opened)
            {
                ModelIndexQuery q{query,model->model.indexId(query.indexT())};
                return afterRead(model,doFindUpdateCreate(*model->info,q,request,object,returnType,tx),TimePointFilter{});
            }

            HATN_CTX_SCOPE_LOCK()
            return dbError(DbError::DB_NOT_OPEN);
        }

        template <typename ModelT, typename QueryT>
        Result<typename ModelT::SharedPtr> findUpdate(
            const std::shared_ptr<ModelT>& model,
            const QueryT& query,
            const update::Request& request,
            update::ModifyReturn returnType=update::ModifyReturn::After,
            Transaction* tx=nullptr)
        {
            HATN_CTX_SCOPE("dbfindupdate")
            if (m_opened)
            {
                ModelIndexQuery q{query,model->model.indexId(query.indexT())};
                return afterRead(model,doFindUpdateCreate(*model->info,q,request,
                                                           HATN_COMMON_NAMESPACE::SharedPtr<dataunit::Unit>{},
                                                           returnType,tx),TimePointFilter{});
            }

            HATN_CTX_SCOPE_LOCK()
            return dbError(DbError::DB_NOT_OPEN);
        }

        Error transaction(const TransactionFn& fn)
        {
            return doTransaction(fn);
        }

    protected:

        virtual Error doCreateDb(const ClientConfig& config, base::config_object::LogRecords& records)=0;
        virtual Error doDestroyDb(const ClientConfig& config, base::config_object::LogRecords& records)=0;

        virtual void doOpenDb(const ClientConfig& config, Error& ec, base::config_object::LogRecords& records)=0;
        virtual void doCloseDb(Error& ec)=0;

        virtual Error doSetSchema(std::shared_ptr<Schema> schema)=0;
        virtual Result<std::shared_ptr<Schema>> doGetSchema() const=0;
        virtual Error doCheckSchema()=0;
        virtual Error doMigrateSchema()=0;

        virtual Error doAddDatePartitions(const std::vector<ModelInfo>& models, const std::set<common::DateRange>& dateRanges)=0;
        virtual Error doDeleteDatePartitions(const std::vector<ModelInfo>& models, const std::set<common::DateRange>& dateRanges)=0;

        virtual Error doCreate(const Topic& topic, const ModelInfo& model, dataunit::Unit* object, Transaction* tx)=0;

        virtual Result<common::SharedPtr<dataunit::Unit>> doRead(const Topic& topic,
                                                                 const ModelInfo& model,
                                                                 const ObjectId& id,
                                                                 Transaction* tx,
                                                                 bool forUpdate
        )=0;
        virtual Result<common::SharedPtr<dataunit::Unit>> doRead(const Topic& topic,
                                                                 const ModelInfo& model,
                                                                 const ObjectId& id,
                                                                 const common::Date& date,
                                                                 Transaction* tx,
                                                                 bool forUpdate
                                                                 )=0;

        virtual Error doDeleteObject(const Topic& topic,
                           const ModelInfo& model,
                           const ObjectId& id,
                           const common::Date& date,
                           Transaction* tx)=0;

        virtual Error doDeleteObject(const Topic& topic,
                                     const ModelInfo& model,
                                     const ObjectId& id,
                                     Transaction* tx)=0;

        virtual Error doUpdateObject(const Topic& topic,
                             const ModelInfo& model,
                             const update::Request& request,
                             const ObjectId& id,
                             const common::Date& date,
                             Transaction* tx)=0;

        virtual Error doUpdateObject(const Topic& topic,
                               const ModelInfo& model,
                               const update::Request& request,
                               const ObjectId& id,
                               Transaction* tx)=0;

        virtual Result<common::SharedPtr<dataunit::Unit>> doReadUpdate(const Topic& topic,
                                     const ModelInfo& model,
                                     const ObjectId& id,
                                     const update::Request& request,                                     
                                     const common::Date& date,
                                     update::ModifyReturn returnType,
                                     Transaction* tx)=0;

        virtual Result<common::SharedPtr<dataunit::Unit>> doReadUpdate(const Topic& topic,
                                                                       const ModelInfo& model,
                                                                       const ObjectId& id,
                                                                       const update::Request& request,                                                                       
                                                                       update::ModifyReturn returnType,
                                                                       Transaction* tx)=0;

        virtual Result<HATN_COMMON_NAMESPACE::pmr::vector<UnitWrapper>> doFind(
            const ModelInfo& model,
            const ModelIndexQuery& query,
            bool single
        ) =0;

        virtual Result<size_t> doCount(
            const ModelInfo& model,
            const ModelIndexQuery& query
        ) =0;

        virtual Result<size_t> doUpdateMany(
            const ModelInfo& model,
            const ModelIndexQuery& query,
            const update::Request& request,
            Transaction* tx
        ) =0;

        virtual Result<common::SharedPtr<dataunit::Unit>> doFindUpdateCreate(
                                                            const ModelInfo& model,
                                                            const ModelIndexQuery& query,
                                                            const update::Request& request,
                                                            const HATN_COMMON_NAMESPACE::SharedPtr<dataunit::Unit>& object,
                                                            update::ModifyReturn returnType,
                                                            Transaction* tx)=0;

        virtual Result<size_t> doDeleteMany(
            const ModelInfo& model,
            const ModelIndexQuery& query,
            Transaction* tx
        ) =0;

        virtual Error doTransaction(const TransactionFn& fn)=0;

        void setClosed() noexcept
        {
            m_opened=false;
        }

    private:

        template <typename ModelT>
        Result<typename ModelT::SharedPtr> afterRead(
            const std::shared_ptr<ModelT>&,
            Result<common::SharedPtr<dataunit::Unit>>&& r,
            const TimePointFilter& tpFilter
            )
        {
            static typename ModelT::ManagedType sample;
            if (r)
            {
                return r.takeError();
            }
            if (r.value().isNull())
            {
                return typename ModelT::SharedPtr{};
            }
            auto* obj=common::dynamicCastWithSample(r.value().get(),&sample);
            if (tpFilter && !tpFilter.filterTimePoint(*obj))
            {
                HATN_CTX_SCOPE_ERROR("filter-timepoint")
                return dbError(DbError::NOT_FOUND);
            }
            return obj->sharedFromThis();
        }

        bool m_opened;
};

HATN_DB_NAMESPACE_END

#endif // HATNDBCLIENT_H_H
