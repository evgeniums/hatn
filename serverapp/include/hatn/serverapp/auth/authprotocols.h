/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*
    
*/
/** @file serverapp/auth/authprotocols.h
  */

/****************************************************************************/

#ifndef HATNAUTHPROTOCOLS_H
#define HATNAUTHPROTOCOLS_H

#include <hatn/common/flatmap.h>

#include <hatn/clientserver/clientservererror.h>

#include <hatn/serverapp/serverappdefs.h>
#include <hatn/serverapp/auth/authprotocol.h>

HATN_SERVERAPP_NAMESPACE_BEGIN

class AuthProtocols
{
    public:

        constexpr static const uint32_t DefaultProtocolPriority=1000;

    protected:

        struct Protocol
        {
            uint32_t priority;
            common::FlatSet<uint32_t> versions;

            Protocol(uint32_t priority=DefaultProtocolPriority) : priority(priority)
            {}
        };

    public:

        AuthProtocols() : m_defaultProtocol(
              HATN_CLIENT_SERVER_NAMESPACE::AUTH_PROTOCOL_HATN_SHARED_SECRET,
              HATN_CLIENT_SERVER_NAMESPACE::AUTH_PROTOCOL_HATN_SHARED_SECRET_VERSION
          )
        {}

        Result<AuthProtocol> negotiate(
                const HATN_CLIENT_SERVER_NAMESPACE::auth_negotiate_request::managed* message
            )
        {
            uint32_t priority=0;
            AuthProtocol p;
            AuthProtocol* proto=nullptr;

            // find matching protocol with highest priority
            const auto& protocols=message->field(HATN_CLIENT_SERVER_NAMESPACE::auth_negotiate_request::protocols);
            for (size_t i=0;i<protocols.count();i++)
            {
                const auto& protocol=protocols.at(i);
                auto it=m_protocols.find(protocol.fieldValue(HATN_API_NAMESPACE::auth_protocol::protocol));
                if (it!=m_protocols.end())
                {
                    auto it1=it->second.versions.find(protocol.fieldValue(HATN_API_NAMESPACE::auth_protocol::version));
                    if (it1!=it->second.versions.end())
                    {
                        if (it->second.priority>priority || priority==0)
                        {
                            if (proto==nullptr || proto->name()!=it->first || proto->version()>*it1)
                            {
                                proto=&p;
                                p.setName(it->first);
                                p.setVersion(*it1);
                            }
                            priority=it->second.priority;
                        }
                    }
                }

                // if protocol not found then check default protocol
                if (proto==nullptr)
                {
                    if (protocol.fieldValue(HATN_API_NAMESPACE::auth_protocol::protocol)==m_defaultProtocol.name()
                        &&
                        protocol.fieldValue(HATN_API_NAMESPACE::auth_protocol::version)==m_defaultProtocol.version()
                        )
                    {
                        proto=&m_defaultProtocol;
                    }
                }
            }

            // failed to negotiate protocol
            if (proto==nullptr)
            {
                return HATN_CLIENT_SERVER_NAMESPACE::clientServerError(HATN_CLIENT_SERVER_NAMESPACE::ClientServerError::AUTH_NEGOTIATION_FAILED);
            }

            // protocol negotiated
            return *proto;
        }

        void registerProtocol(
            std::string name,
            uint32_t version=1,
            uint32_t priority=DefaultProtocolPriority
            )
        {
            Protocol* p=nullptr;
            auto it=m_protocols.find(name);
            if (it!=m_protocols.end())
            {
                p=&it->second;
            }
            else
            {
                auto it1=m_protocols.emplace(std::move(name),Protocol{priority});
                p=&it1.first->second;
            }
            p->versions.insert(version);
            p->priority=priority;
        }

        void registerProtocol(
            const AuthProtocol& protocol,
            uint32_t priority=DefaultProtocolPriority
            )
        {
            registerProtocol(protocol.name(),protocol.version(),priority);
        }

        void setDefaultProtocol(AuthProtocol defaultProtocol)
        {
            m_defaultProtocol=std::move(defaultProtocol);
        }

        const AuthProtocol& defaultProtocol() const
        {
            return m_defaultProtocol;
        }

    private:

        common::FlatMap<std::string,Protocol> m_protocols;
        AuthProtocol m_defaultProtocol;
};

HATN_SERVERAPP_NAMESPACE_END

#endif // HATNAUTHPROTOCOLS_H
