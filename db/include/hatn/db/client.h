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

#include <hatn/base/configtree.h>
#include <hatn/base/configobject.h>

#include <hatn/db/db.h>
#include <hatn/db/dberror.h>
#include <hatn/db/model.h>
#include <hatn/db/schema.h>

#include <hatn/db/namespace.h>

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
            if (m_opened)
            {
                return DbError::DB_ALREADY_OPEN;
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
            return doCreateDb(config,records);
        }

        Error destroyDb(const ClientConfig& config, base::config_object::LogRecords& records)
        {
            return doDestroyDb(config,records);
        }

        Error addSchema(std::shared_ptr<DbSchema> schema)
        {
            if (m_opened)
            {
                return doAddSchema(std::move(schema));
            }
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
            if (m_opened)
            {
                return doListSchemas();
            }
            return dbError(DbError::DB_NOT_OPEN);
        }

        Result<std::shared_ptr<DbSchema>> schema(const common::lib::string_view& schemaName) const
        {
            if (m_opened)
            {
                return doFindSchema(schemaName);
            }
            return dbError(DbError::DB_NOT_OPEN);
        }

        Error checkSchemas()
        {
            if (m_opened)
            {
                return doCheckSchemas();
            }
            return dbError(DbError::DB_NOT_OPEN);
        }

        Error migrateSchemas()
        {
            if (m_opened)
            {
                return doMigrateSchemas();
            }
            return dbError(DbError::DB_NOT_OPEN);
        }

        Error addDatePartitions(const std::vector<ModelInfo>& models, const common::Date& to, const common::Date& from=common::Date{})
        {
            if (m_opened)
            {
                return doAddDatePartitions(models,datePartitionRanges(models,to,from));
            }
            return dbError(DbError::DB_NOT_OPEN);
        }

        Error deleteDatePartitions(const std::vector<ModelInfo>& models, const common::Date& to, const common::Date& from=common::Date{})
        {
            if (m_opened)
            {
                return doDeleteDatePartitions(models,datePartitionRanges(models,to,from));
            }
            return dbError(DbError::DB_NOT_OPEN);
        }

        static std::set<common::DateRange> datePartitionRanges(const std::vector<ModelInfo>& models, const common::Date& to, const common::Date& from=common::Date{});

        template <typename ModelT>
        Error create(const db::Namespace& ns, const std::shared_ptr<ModelT>& model, dataunit::Unit* object)
        {
            if (m_opened)
            {
                return doCreate(ns,model->info,object);
            }
            return dbError(DbError::DB_NOT_OPEN);
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

        virtual Error doCreate(const db::Namespace& ns, const ModelInfo& model, dataunit::Unit* object)=0;

        void setClosed() noexcept
        {
            m_opened=false;
        }

    private:

        bool m_opened;
};

HATN_DB_NAMESPACE_END

#endif // HATNDBCLIENT_H_H
