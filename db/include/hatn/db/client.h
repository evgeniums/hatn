/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
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

HATN_DB_NAMESPACE_BEGIN

struct ClientConfig
{
    const base::ConfigTree& main;
    const base::ConfigTree& opts;

    base::ConfigTreePath mainPath;
    base::ConfigTreePath optsPath;
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

        Error open(const ClientConfig& config, base::config_object::LogRecords& records)
        {
            if (m_opened)
            {
                return DbError::ALREADY_OPENED;
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
            doOpen(config,ec,records);
            return ec;
        }

        Error close()
        {
            if (m_opened)
            {
                Error ec;
                HATN_SCOPE_GUARD([this](){m_opened=false;})
                doClose(ec);
                return ec;
            }
            return OK;
        }

    protected:

        virtual void doOpen(const ClientConfig& config, Error& ec, base::config_object::LogRecords& records)=0;
        virtual void doClose(Error& ec)=0;

        void setClosed() noexcept
        {
            m_opened=false;
        }

    private:

        bool m_opened;
};

HATN_DB_NAMESPACE_END

#endif // HATNDBCLIENT_H_H
