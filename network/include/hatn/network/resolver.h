/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file network/resolver.h
  *
  *   DNS resolver
  *
  */

/****************************************************************************/

#ifndef HATNNETWORKRESOLVER_H
#define HATNNETWORKRESOLVER_H

#include <functional>
#include <memory>

#include <hatn/common/error.h>
#include <hatn/common/fixedbytearray.h>
#include <hatn/common/objectid.h>
#include <hatn/common/logger.h>
#include <hatn/common/objecttraits.h>

#include <hatn/network/network.h>
#include <hatn/network/networkerror.h>
#include <hatn/network/asio/ipendpoint.h>

DECLARE_LOG_MODULE_EXPORT(dnsresolver,HATN_NETWORK_EXPORT)

HATN_NETWORK_NAMESPACE_BEGIN

//! Nameserver
struct NameServer
{
    asio::IpAddress address;
    uint16_t udpPort=53;
    uint16_t tcpPort=53;

    NameServer(const char* addr):address(asio::IpAddress::from_string(addr))
    {}

    NameServer(const std::string& addr):address(asio::IpAddress::from_string(addr))
    {}
};

/**
 * @brief DNS resolver
 *
 * It can be used to resolve hostnames into IPv4 and/or IPv6 addresses as well as to resolve SRV records.
 */
template <typename Traits>
class Resolver : public common::WithTraits<Traits>,
                 public common::WithIDThread
{
    public:

        //! Ctor
        template <typename ...Args>
        Resolver(
                common::Thread* thread,
                common::STR_ID_TYPE id,
                Args&& ...traitsArgs
        ) : common::WithTraits<Traits>(std::forward<Args>(traitsArgs)...),
            common::WithIDThread(thread,std::move(id))
        {}

        //! Ctor
        template <typename ...Args>
        Resolver(
                common::Thread* thread,
                Args&& ...traitsArgs
        ) : Resolver(thread,common::STR_ID_TYPE(),std::forward<Args>(traitsArgs)...)
        {}

        //! Dtor
        ~Resolver()
        {
            cancel();
            cleanup();
        }

        Resolver(const Resolver&)=delete;
        Resolver(Resolver&&) =default;
        Resolver& operator=(const Resolver&)=delete;
        Resolver& operator=(Resolver&&) =default;

        /**
         * @brief Resolve host by name as defined in either system hosts files or in A or in AAAA or in CNAME records
         * @param hostName Name of the host
         * @param callback Callback to invoke with query result
         * @param port Port number to use in result endpoints
         * @param ipVersion Version of IP protocol to use in queries and to filter result
         */
        void resolveName(
            const char* hostName,
            std::function<void (const common::Error&,std::vector<asio::IpEndpoint>)> callback,
            uint16_t port=0,
            IpVersion ipVersion=IpVersion::ALL
        )
        {
            this->traits().resolveName(hostName,std::move(callback),port,ipVersion);
        }

        /**
         * @brief Resolve host by service as defined in SRV records
         * @param name Full name of the service, e.g. _sip._tcp.example.com
         * @param callback Callback to invoke with query result
         * @param ipVersion Version of IP protocol to use in query to resolve hostnames or filter addresses from SRV records
         */
        void resolveService(
            const char* name,
            std::function<void (const common::Error&,std::vector<asio::IpEndpoint>)> callback,
            IpVersion ipVersion=IpVersion::ALL
        )
        {
            this->traits().resolveService(name,std::move(callback),ipVersion);
        }

        /**
         * @brief Resolve mx records
         * @param name Full name of the service, e.g. _sip._tcp.example.com
         * @param callback Callback to invoke with query result
         * @param ipVersion Version of IP protocol to use in query to resolve hostnames or filter addresses from SRV records
         */
        void resolveMx(
            const char* name,
            std::function<void (const common::Error&,std::vector<asio::IpEndpoint>)> callback,
            IpVersion ipVersion=IpVersion::ALL
        )
        {
            this->traits().resolveMx(name,std::move(callback),ipVersion);
        }

        //! Cancel all pending queries
        void cancel()
        {
            this->traits().cancel();
        }

        //! Cleanup the object
        void cleanup()
        {
            this->traits().cleanup();
        }
};

//! Base polymorphic resolver class
class ResolverV
{
    public:

        /**
         * @brief Make service name to use in query for SRV record
         * @param domain
         * @param service
         * @param protocol
         * @return Formatted name
         */
        inline static common::FixedByteArrayThrow256 makeServiceName(
                const char* domain,
                const char* service,
                IpProtocol protocol
            )
        {
            common::FixedByteArrayThrow256 result;
            result.append('_');
            result.append(service);
            result.append('.');
            if (protocol==IpProtocol::TCP)
            {
                result.append("_tcp");
            }
            else
            {
                result.append("_udp");
            }
            result.append('.');
            result.append(domain);
            return result;
        }

        ResolverV()=default;
        virtual ~ResolverV()=default;
        ResolverV(const ResolverV&)=delete;
        ResolverV(ResolverV&&) =default;
        ResolverV& operator=(const ResolverV&)=delete;
        ResolverV& operator=(ResolverV&&) =default;

        /**
         * @brief Resolve host by name as defined in either system hosts files or in A or in AAAA or in CNAME records
         * @param hostName Name of the host
         * @param callback Callback to invoke with query result
         * @param port Port number to use in result endpoints
         * @param ipVersion Version of IP protocol to use in queries and to filter result
         */
        virtual void resolveName(
            const char* hostName,
            std::function<void (const common::Error&,std::vector<asio::IpEndpoint>)> callback,
            uint16_t port=0,
            IpVersion ipVersion=IpVersion::ALL
        )=0;

        /**
         * @brief Resolve host by service as defined in SRV records
         * @param name Full name of the service, e.g. _sip._tcp.example.com
         * @param callback Callback to invoke with query result
         * @param ipVersion Version of IP protocol to use in query to resolve hostnames or filter addresses from SRV records
         */
        virtual void resolveService(
            const char* name,
            std::function<void (const common::Error&,std::vector<asio::IpEndpoint>)> callback,
            IpVersion ipVersion=IpVersion::ALL
        ) =0;

        /**
         * @brief Resolve mx records
         * @param name Full name of the service, e.g. _sip._tcp.example.com
         * @param callback Callback to invoke with query result
         * @param ipVersion Version of IP protocol to use in query to resolve hostnames or filter addresses from SRV records
         */
        virtual void resolveMx(
            const char* name,
            std::function<void (const common::Error&,std::vector<asio::IpEndpoint>)> callback,
            IpVersion ipVersion=IpVersion::ALL
        ) =0;

        //! Cancel all pending queries
        virtual void cancel()=0;
};

//! Base template for building polymorphic resolvers with implementor traits
template <typename ImplT>
class ResolverTmlV : public common::WithImpl<ImplT>, public ResolverV
{
    public:

        using common::WithImpl<ImplT>::WithImpl;

        /**
         * @brief Resolve host by name as defined in either system hosts files or in A or in AAAA or in CNAME records
         * @param hostName Name of the host
         * @param callback Callback to invoke with query result
         * @param port Port number to use in result endpoints
         * @param ipVersion Version of IP protocol to use in queries and to filter result
         */
        virtual void resolveName(
            const char* hostName,
            std::function<void (const common::Error&,std::vector<asio::IpEndpoint>)> callback,
            uint16_t port=0,
            IpVersion ipVersion=IpVersion::ALL
        ) override
        {
            this->impl().resolveName(hostName,std::move(callback),port,ipVersion);
        }

        /**
         * @brief Resolve host by service as defined in SRV records
         * @param name Full name of the service, e.g. _sip._tcp.example.com
         * @param callback Callback to invoke with query result
         * @param ipVersion Version of IP protocol to use in query to resolve hostnames or filter addresses from SRV records
         */
        virtual void resolveService(
            const char* name,
            std::function<void (const common::Error&,std::vector<asio::IpEndpoint>)> callback,
            IpVersion ipVersion=IpVersion::ALL
        ) override
        {
            this->impl().resolveService(name,std::move(callback),ipVersion);
        }

        /**
         * @brief Resolve mx records
         * @param name Full name of the service, e.g. _sip._tcp.example.com
         * @param callback Callback to invoke with query result
         * @param ipVersion Version of IP protocol to use in query to resolve hostnames or filter addresses from SRV records
         */
        virtual void resolveMx(
            const char* name,
            std::function<void (const common::Error&,std::vector<asio::IpEndpoint>)> callback,
            IpVersion ipVersion=IpVersion::ALL
        ) override
        {
            this->impl().resolveMx(name,std::move(callback),ipVersion);
        }

        //! Cancel all pending queries
        virtual void cancel() override
        {
            this->impl().cancel();
        }
};


HATN_NETWORK_NAMESPACE_END

#endif // HATNNETWORKRESOLVER_H
