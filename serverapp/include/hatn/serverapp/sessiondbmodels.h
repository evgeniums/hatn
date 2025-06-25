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

HDU_UNIT_WITH(session_agent,(HDU_BASE(db::object)),
    HDU_FIELD(agent,TYPE_STRING,1)
    HDU_FIELD(agent_os,TYPE_STRING,2)
    HDU_FIELD(agent_version,TYPE_STRING,3)
    HDU_FIELD(unique_hash,TYPE_STRING,4)
    HDU_FIELD(ttl,TYPE_DATETIME,5)
)

HATN_DB_UNIQUE_INDEX(sessionAgentHashIdx,session_agent::unique_hash)
HATN_DB_TTL_INDEX(sessionAgentTtlIdx,1,session_agent::ttl)
HATN_DB_MODEL_PROTOTYPE(sessionAgentModel,session_agent,sessionAgentHashIdx(),sessionAgentTtlIdx())

HDU_UNIT_WITH(session_endpoint,(HDU_BASE(db::object)),
    HDU_FIELD(session,TYPE_OBJECT_ID,1)
    HDU_FIELD(agent,TYPE_OBJECT_ID,2)
    HDU_FIELD(ip_addr,TYPE_STRING,3)
    HDU_FIELD(ip_port,TYPE_UINT16,4)
    HDU_FIELD(ttl,TYPE_DATETIME,5)
)

HATN_DB_INDEX(sessionEndpointIdx,session_endpoint::session)
HATN_DB_INDEX(sessionEndpointAgentIdx,session_endpoint::agent)
HATN_DB_INDEX(sessionEndpointIpAddrIdx,session_endpoint::ip_addr)
HATN_DB_TTL_INDEX(sessionEndpointTtlIdx,1,session_endpoint::ttl)
HATN_DB_MODEL_PROTOTYPE(sessionEndpointModel,session_endpoint,sessionEndpointIdx(),sessionEndpointAgentIdx(),sessionEndpointIpAddrIdx(),sessionEndpointTtlIdx())

class SessionDbModels : public db::ModelsWrapper
{
    public:

        SessionDbModels(std::string prefix={}) : db::ModelsWrapper(std::move(prefix))
        {}

        const auto& sessionModel() const
        {
            return db::makeModelFromProrotype(prefix(),HATN_SERVERAPP_NAMESPACE::sessionModel);
        }

        const auto& sessionAgentModel() const
        {
            return db::makeModelFromProrotype(prefix(),HATN_SERVERAPP_NAMESPACE::sessionAgentModel);
        }

        const auto& sessionEndpointModel() const
        {
            return db::makeModelFromProrotype(prefix(),HATN_SERVERAPP_NAMESPACE::sessionEndpointModel);
        }

        auto models()
        {
            return hana::make_tuple(
                [this](){return sessionModel();},
                [this](){return sessionAgentModel();},
                [this](){return sessionEndpointModel();}
            );
        }
};

HATN_SERVERAPP_NAMESPACE_END

#endif // HATNSESSIONDBMODELS_H
