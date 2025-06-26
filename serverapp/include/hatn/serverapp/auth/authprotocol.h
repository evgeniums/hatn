/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*
    
*/
/** @file serverapp/auth/authprotocol.h
  */

/****************************************************************************/

#ifndef HATNAUTHPROTOCOL_H
#define HATNAUTHPROTOCOL_H

#include <string>

#include <hatn/clientserver/auth/authprotocol.h>

#include <hatn/serverapp/serverappdefs.h>

HATN_SERVERAPP_NAMESPACE_BEGIN

class AuthProtocol
{
    public:

        AuthProtocol(
            std::string name={},
            uint32_t version=1
        ) : m_name(std::move(name)),
            m_version(std::move(version))
        {}

        uint32_t version() const noexcept
        {
            return m_version;
        }

        const std::string& name() const noexcept
        {
            return m_name;
        }

        void setName(std::string name)
        {
            m_name=std::move(name);
        }

        void setVersion(uint32_t version)
        {
            m_version=version;
        }

        bool is(const AuthProtocol& other) const noexcept
        {
            return name()==other.name() && version()==other.version();
        }

    private:

        std::string m_name;
        uint32_t m_version;
};

HATN_SERVERAPP_NAMESPACE_END

#endif // HATNAUTHPROTOCOL_H
