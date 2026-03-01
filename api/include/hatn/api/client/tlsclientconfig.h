/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file api/client/tlsclientconfig.h
  *
  */

/****************************************************************************/

#ifndef HATNTLSCLIENTCONFIG_H
#define HATNTLSCLIENTCONFIG_H

#include <hatn/common/locker.h>

#include <hatn/api/api.h>
#include <hatn/api/client/tcpclientconfig.h>

HATN_API_NAMESPACE_BEGIN

namespace client {

class TlsClientConfig : public TcpClientConfig
{
    public:

        using TcpClientConfig::TcpClientConfig;

        void setServerCaList(std::vector<std::string> value)
        {
            common::MutexScopedLock l(m_mutex);
            m_serverCaList=std::move(value);
            m_concatenatedServerCerts.clear();
        }

        void setServerCa(std::string value)
        {
            setServerCertChain({std::move(value)});
        }

        void setServerCertChain(std::vector<std::string> value)
        {
            common::MutexScopedLock l(m_mutex);
            m_serverCertChain=std::move(value);
            m_concatenatedServerCerts.clear();
        }

        const std::string& serverCerts() const noexcept
        {
            common::MutexScopedLock l(m_mutex);

            if (m_concatenatedServerCerts.empty())
            {
                for (const auto& cert : m_serverCertChain)
                {
                    concat(m_concatenatedServerCerts,cert);
                }
                for (const auto& cert : m_serverCaList)
                {
                    concat(m_concatenatedServerCerts,cert);
                }
            }
            return m_concatenatedServerCerts;
        }

        void setClientCert(std::string value)
        {
            setClientCertChain({std::move(value)});
        }

        void setClientCertChain(std::vector<std::string> value)
        {
            {
                common::MutexScopedLock l(m_mutex);
                m_clientCertChain=std::move(value);
                m_concatenatedClientCertChains.clear();
            }
            clientCertChain();
        }

        const std::string& clientCertChain() const noexcept
        {
            common::MutexScopedLock l(m_mutex);

            if (m_concatenatedClientCertChains.empty())
            {
                for (const auto& cert : m_clientCertChain)
                {
                    concat(m_concatenatedClientCertChains,cert);
                }
            }
            return m_concatenatedClientCertChains;
        }

        void setClientPrivKey(std::string value)
        {
            common::MutexScopedLock l(m_mutex);
            m_clientPrivKey=std::move(value);
        }

        const std::string& clientPrivKey() const noexcept
        {
            common::MutexScopedLock l(m_mutex);
            return m_clientPrivKey;
        }

        bool isInsecure() const noexcept
        {
            common::MutexScopedLock l(m_mutex);
            return m_insecure;
        }

        void setInsecure(bool enable) noexcept
        {
            common::MutexScopedLock l(m_mutex);
            m_insecure=enable;
        }

    private:

        static void concat(std::string& bundle, const std::string& cert)
        {
            bundle += cert;
            if (cert.back() != '\n')
            {
                bundle += "\n";
            }
        }

        mutable common::MutexLock m_mutex;

        std::vector<std::string> m_serverCaList;
        std::vector<std::string> m_serverCertChain;
        std::vector<std::string> m_clientCertChain;

        mutable std::string m_concatenatedServerCerts;
        mutable std::string m_concatenatedClientCertChains;

        std::string m_clientPrivKey;

        bool m_insecure=false;
};

} // namespace client

HATN_API_NAMESPACE_END

#endif // HATNTLSCLIENTCONFIG_H
