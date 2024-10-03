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
#include <hatn/db/namespace.h>
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

        Error addSchema(std::shared_ptr<DbSchema> schema)
        {
            HATN_CTX_SCOPE("dbaddschema")
            if (m_opened)
            {
                return doAddSchema(std::move(schema));
            }
            HATN_CTX_SCOPE_LOCK()
            return dbError(DbError::DB_NOT_OPEN);
        }

        template <typename T>
        Error addSchema(std::shared_ptr<T> schema,
                        std::enable_if_t<
                            !std::is_same<T,DbSchema>::value,void*
                            > =nullptr
            )
        {
            return addSchema(std::static_pointer_cast<DbSchema>(schema));
        }

        Result<std::vector<std::shared_ptr<DbSchema>>> listSchemas() const
        {
            HATN_CTX_SCOPE("dblistschemas")
            if (m_opened)
            {
                return doListSchemas();
            }
            HATN_CTX_SCOPE_LOCK()
            return dbError(DbError::DB_NOT_OPEN);
        }

        Result<std::shared_ptr<DbSchema>> schema(const common::lib::string_view& schemaName) const
        {
            HATN_CTX_SCOPE("dbschema")
            if (m_opened)
            {
                return doFindSchema(schemaName);
            }
            HATN_CTX_SCOPE_LOCK()
            return dbError(DbError::DB_NOT_OPEN);
        }

        Error checkSchemas()
        {
            HATN_CTX_SCOPE("dbcheckschemas")
            if (m_opened)
            {
                return doCheckSchemas();
            }
            HATN_CTX_SCOPE_LOCK()
            return dbError(DbError::DB_NOT_OPEN);
        }

        Error migrateSchemas()
        {
            HATN_CTX_SCOPE("dbmigrateschemas")
            if (m_opened)
            {
                return doMigrateSchemas();
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
        Error create(const Namespace& ns,
                     const std::shared_ptr<ModelT>& model,
                     dataunit::Unit* object,
                     Transaction* tx=nullptr
                     )
        {
            HATN_CTX_SCOPE("dbcreate")
            if (m_opened)
            {
                return doCreate(ns,model->info,object,tx);
            }
            HATN_CTX_SCOPE_LOCK()
            return dbError(DbError::DB_NOT_OPEN);
        }

        //! @todo Add Read for update.

        //! @todo Read from transaction

        template <typename ModelT>
        Result<typename ModelT::SharedPtr> read(const Namespace& ns,
                                                const std::shared_ptr<ModelT>& model,
                                                const ObjectId& id,
                                                const TimePointFilter& tpFilter=TimePointFilter{})
        {
            static typename ModelT::type sample;

            HATN_CTX_SCOPE("dbread")
            if (m_opened)
            {
                auto r=doRead(ns,model->info,id);
                if (r)
                {
                    return r.takeError();
                }
                auto* obj=sample.castToUnit(r.value().get());
                if (!tpFilter.filterTimePoint(*obj))
                {
                    HATN_CTX_SCOPE_ERROR("filter-timepoint")
                    return dbError(DbError::NOT_FOUND);
                }
                return obj->sharedFromThis();
            }

            HATN_CTX_SCOPE_LOCK()
            return dbError(DbError::DB_NOT_OPEN);
        }

        template <typename ModelT>
        Result<typename ModelT::SharedPtr> read(const Namespace& ns,
                                                const std::shared_ptr<ModelT>& model,
                                                const ObjectId& id,
                                                const common::Date& date,
                                                const TimePointFilter& tpFilter=TimePointFilter{})
        {
            static typename ModelT::type sample;

            HATN_CTX_SCOPE("dbreaddate")
            if (m_opened)
            {
                auto r=doRead(ns,model->info,id,date);
                if (r)
                {
                    return r.takeError();
                }
                auto* obj=sample.castToUnit(r.value().get());
                if (!tpFilter.filterTimePoint(*obj))
                {
                    HATN_CTX_SCOPE_ERROR("filter-timepoint")
                    return dbError(DbError::NOT_FOUND);
                }
                return obj->sharedFromThis();
            }

            HATN_CTX_SCOPE_LOCK()
            return dbError(DbError::DB_NOT_OPEN);
        }

        template <typename ModelT>
        Error updateObject(const Namespace& ns,
                       const std::shared_ptr<ModelT>& model,
                       const update::Request& request,
                       const ObjectId& id,
                       const common::Date& date,
                       Transaction* tx=nullptr)
        {
            HATN_CTX_SCOPE("dbupdate")
            if (m_opened)
            {
                return doUpdateObject(ns,model->info,request,id,date,tx);
            }

            HATN_CTX_SCOPE_LOCK()
            return dbError(DbError::DB_NOT_OPEN);
        }

        template <typename ModelT>
        Error updateObject(const Namespace& ns,
                     const std::shared_ptr<ModelT>& model,
                     const update::Request& request,
                     const ObjectId& id,
                     Transaction* tx=nullptr)
        {
            HATN_CTX_SCOPE("dbupdate")
            if (m_opened)
            {
                return doUpdateObject(ns,model->info,request,id,tx);
            }

            HATN_CTX_SCOPE_LOCK()
            return dbError(DbError::DB_NOT_OPEN);
        }

        template <typename ModelT>
        Error deleteObject(const Namespace& ns,
                            const std::shared_ptr<ModelT>& model,
                            const ObjectId& id,
                            const common::Date& date,
                            Transaction* tx=nullptr)
        {
            HATN_CTX_SCOPE("dbdelete")
            if (m_opened)
            {
                return doDeleteObject(ns,model->info,id,date,tx);
            }

            HATN_CTX_SCOPE_LOCK()
            return dbError(DbError::DB_NOT_OPEN);
        }

        template <typename ModelT>
        Error deleteObject(const Namespace& ns,
                            const std::shared_ptr<ModelT>& model,
                            const ObjectId& id,
                            Transaction* tx=nullptr)
        {
            HATN_CTX_SCOPE("dbdelete")
            if (m_opened)
            {
                return doDeleteObject(ns,model->info,id,tx);
            }

            HATN_CTX_SCOPE_LOCK()
            return dbError(DbError::DB_NOT_OPEN);
        }

        template <typename ModelT>
        Result<HATN_COMMON_NAMESPACE::pmr::vector<UnitWrapper>> find(
            const Namespace& ns,
            const std::shared_ptr<ModelT>& model,
            IndexQuery& query
        )
        {
            HATN_CTX_SCOPE("dbfind")
            if (m_opened)
            {
                return doFind(ns,model->info,query);
            }

            HATN_CTX_SCOPE_LOCK()
            return dbError(DbError::DB_NOT_OPEN);
        }

        template <typename ModelT>
        Error deleteMany(
                const Namespace& ns,
                const std::shared_ptr<ModelT>& model,
                IndexQuery& query,
                Transaction* tx=nullptr
            )
        {
            HATN_CTX_SCOPE("dbdeletemany")
            if (m_opened)
            {
                return doDeleteMany(ns,model->info,query,tx);
            }

            HATN_CTX_SCOPE_LOCK()
            return dbError(DbError::DB_NOT_OPEN);
        }

        template <typename ModelT>
        Error updateMany(
                const Namespace& ns,
                const std::shared_ptr<ModelT>& model,
                IndexQuery& query,
                const update::Request& request,
                Transaction* tx=nullptr
            )
        {
            HATN_CTX_SCOPE("dbupdatemany")
            if (m_opened)
            {
                return doUpdateMany(ns,model->info,query,request,tx);
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

        virtual Error doAddSchema(std::shared_ptr<DbSchema> schema)=0;
        virtual Result<std::shared_ptr<DbSchema>> doFindSchema(const common::lib::string_view& schemaName) const=0;
        virtual Result<std::vector<std::shared_ptr<DbSchema>>> doListSchemas() const=0;
        virtual Error doCheckSchemas()=0;
        virtual Error doMigrateSchemas()=0;

        virtual Error doAddDatePartitions(const std::vector<ModelInfo>& models, const std::set<common::DateRange>& dateRanges)=0;
        virtual Error doDeleteDatePartitions(const std::vector<ModelInfo>& models, const std::set<common::DateRange>& dateRanges)=0;

        virtual Error doCreate(const Namespace& ns, const ModelInfo& model, dataunit::Unit* object, Transaction* tx)=0;

        virtual Result<common::SharedPtr<dataunit::Unit>> doRead(const Namespace& ns, const ModelInfo& model, const ObjectId& id)=0;
        virtual Result<common::SharedPtr<dataunit::Unit>> doRead(const Namespace& ns, const ModelInfo& model, const ObjectId& id, const common::Date& date)=0;

        virtual Error doDeleteObject(const Namespace& ns,
                           const ModelInfo& model,
                           const ObjectId& id,
                           const common::Date& date,
                           Transaction* tx)=0;

        virtual Error doDeleteObject(const Namespace& ns,
                                     const ModelInfo& model,
                                     const ObjectId& id,
                                     Transaction* tx)=0;

        virtual Error doUpdateObject(const Namespace& ns,
                             const ModelInfo& model,
                             const update::Request& request,
                             const ObjectId& id,
                             const common::Date& date,
                             Transaction* tx)=0;

        virtual Error doUpdateObject(const Namespace& ns,
                               const ModelInfo& model,
                               const update::Request& request,
                               const ObjectId& id,
                               Transaction* tx)=0;

        virtual Result<HATN_COMMON_NAMESPACE::pmr::vector<UnitWrapper>> doFind(
            const Namespace& ns,
            const ModelInfo& model,
            IndexQuery& query
        ) =0;

        virtual Error doUpdateMany(
            const Namespace& ns,
            const ModelInfo& model,
            IndexQuery& query,
            const update::Request& request,
            Transaction* tx
        ) =0;

        virtual Error doDeleteMany(
            const Namespace& ns,
            const ModelInfo& model,
            IndexQuery& query,
            Transaction* tx
        ) =0;

        virtual Error doTransaction(const TransactionFn& fn)=0;

        void setClosed() noexcept
        {
            m_opened=false;
        }

    private:

        bool m_opened;
};

HATN_DB_NAMESPACE_END

#endif // HATNDBCLIENT_H_H
