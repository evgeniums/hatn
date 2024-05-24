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

#include <hatn/common/error.h>
#include <hatn/common/runonscopeexit.h>
#include <hatn/common/objectid.h>

#include <hatn/base/configtree.h>
#include <hatn/base/configobject.h>

#include <hatn/db/db.h>
#include <hatn/db/dberror.h>

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

        using common::WithID::WithID;

        Client(const Client&)=delete;
        Client(Client&&)=default;
        Client& operator=(const Client&)=delete;
        Client& operator=(Client&&)=default;

        virtual ~Client();

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

        Error checkSchema(const std::string& schemaName, const Namespace& ns)
        {
            if (m_opened)
            {
                return doCheckSchema(schemaName,ns);
            }
            return dbError(DbError::DB_NOT_OPEN);
        }

        Error migrateSchema(const std::string& schemaName, const Namespace& ns)
        {
            if (m_opened)
            {
                return doMigrateSchema(schemaName,ns);
            }
            return dbError(DbError::DB_NOT_OPEN);
        }

    protected:

        virtual Error doCreateDb(const ClientConfig& config, base::config_object::LogRecords& records)=0;
        virtual Error doDestroyDb(const ClientConfig& config, base::config_object::LogRecords& records)=0;

        virtual void doOpenDb(const ClientConfig& config, Error& ec, base::config_object::LogRecords& records)=0;
        virtual void doCloseDb(Error& ec)=0;

        virtual Error doCheckSchema(const std::string& schemaName, const Namespace& ns)=0;
        virtual Error doMigrateSchema(const std::string& schemaName, const Namespace& ns)=0;

        void setClosed() noexcept
        {
            m_opened=false;
        }

    private:

        bool m_opened;
};

HATN_DB_NAMESPACE_END

#endif // HATNDBCLIENT_H_H
