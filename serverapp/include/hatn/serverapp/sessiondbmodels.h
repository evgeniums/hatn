/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*
    
*/
/** @file serverapp/sessiondbmodels.h
  */

/****************************************************************************/

#ifndef HATNSESSIONDBMODELS_H
#define HATNSESSIONDBMODELS_H

#include <hatn/db/model.h>
#include <hatn/db/modelswrapper.h>

#include <hatn/serverapp/serverappdefs.h>

HATN_SERVERAPP_NAMESPACE_BEGIN

HDU_UNIT_WITH(session,(HDU_BASE(db::object)),
    HDU_FIELD(login,TYPE_OBJECT_ID,1)
    HDU_FIELD(active,TYPE_BOOL,2,false,true)
    HDU_FIELD(ttl,TYPE_DATETIME,3)
    HDU_FIELD(username,TYPE_STRING,4)
)

HATN_DB_INDEX(sessionLoginIdx,session::login)
HATN_DB_TTL_INDEX(sessionTtlIdx,1,session::ttl)
HATN_DB_INDEX(sessionActiveIdx,session::active)
HATN_DB_MODEL_PROTOTYPE(sessionModel,session,sessionLoginIdx(),sessionTtlIdx(),sessionActiveIdx())

struct ClientAgent
{
    //! @todo optimization: Use strings on stack

    std::string name;
    std::string os;
    std::string version;
};

struct ClientIp
{
    //! @todo optimization: Use strings on stack

    std::string ip_addr;
    uint16_t ip_port;
    std::string proxy_ip_addr;
    uint16_t proxy_ip_port;
};

HDU_UNIT_WITH(session_client,(HDU_BASE(db::object)),
    HDU_FIELD(login,TYPE_OBJECT_ID,1)
    HDU_FIELD(session,TYPE_OBJECT_ID,2)
    HDU_FIELD(agent,TYPE_STRING,3)
    HDU_FIELD(agent_os,TYPE_STRING,4)
    HDU_FIELD(agent_version,TYPE_STRING,5)
    HDU_FIELD(ip_addr,TYPE_STRING,6)
    HDU_FIELD(proxy_ip_addr,TYPE_STRING,7)
    HDU_FIELD(ttl,TYPE_DATETIME,8)
)

HATN_DB_INDEX(sessionClientSessIdx,session_client::session,session_client::ip_addr,session_client::proxy_ip_addr)
HATN_DB_INDEX(sessionClientLoginIdx,session_client::login,session_client::ip_addr)
HATN_DB_INDEX(sessionClientLoginProxyIdx,session_client::login,session_client::proxy_ip_addr)
HATN_DB_INDEX(sessionClientAgentIdx,session_client::agent_os,session_client::agent,session_client::agent_version)
HATN_DB_INDEX(sessionClientIpAddrIdx,session_client::ip_addr)
HATN_DB_INDEX(sessionClientProxyIpAddrIdx,session_client::proxy_ip_addr)
HATN_DB_TTL_INDEX(sessionClientTtlIdx,1,session_client::ttl)
HATN_DB_MODEL_PROTOTYPE(sessionClientModel,session_client,
                        sessionClientSessIdx(),
                        sessionClientLoginIdx(),
                        sessionClientAgentIdx(),
                        sessionClientIpAddrIdx(),
                        sessionClientTtlIdx(),
                        sessionClientLoginProxyIdx(),
                        sessionClientProxyIpAddrIdx()
                )

class SessionDbModels : public db::ModelsWrapper
{
    public:

        SessionDbModels(std::string prefix={}) : db::ModelsWrapper(std::move(prefix))
        {}

        const auto& sessionModel() const
        {
            return db::makeModelFromProrotype(prefix(),HATN_SERVERAPP_NAMESPACE::sessionModel);
        }

        const auto& sessionClientModel() const
        {
            return db::makeModelFromProrotype(prefix(),HATN_SERVERAPP_NAMESPACE::sessionClientModel);
        }

        auto models()
        {
            return hana::make_tuple(
                [this](){return sessionModel();},
                [this](){return sessionClientModel();}
            );
        }
};

HATN_SERVERAPP_NAMESPACE_END

#endif // HATNSESSIONDBMODELS_H
