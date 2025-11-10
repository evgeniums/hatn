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
#include <hatn/common/objecttraits.h>
#include <hatn/common/withthread.h>
#include <hatn/common/taskcontext.h>
#include <hatn/common/allocatoronstack.h>

#include <hatn/network/network.h>
#include <hatn/network/networkerror.h>
#include <hatn/network/asio/ipendpoint.h>

HATN_NETWORK_NAMESPACE_BEGIN

//! Nameserver
struct NameServer
{
    asio::IpAddress::type address;
    uint16_t udpPort=53;
    uint16_t tcpPort=53;

    template <typename T>
    NameServer(T&& addr):address(asio::makeAddress(std::forward<T>(addr)))
    {}
};

using ResolverCallback=std::function<void (const Error&,std::vector<asio::IpEndpoint>)>;

/**
 * @brief DNS resolver
 *
 * It can be used to resolve hostnames into IPv4 and/or IPv6 addresses as well as to resolve SRV records.
 */
template <typename Traits>
class Resolver : public common::WithThread,
                 public common::WithTraits<Traits>
{
    public:

        //! Ctor
        template <typename ...Args>
        explicit Resolver(
                common::Thread* thread,
                Args&& ...traitsArgs
        ) : common::WithThread(thread),
            common::WithTraits<Traits>(std::forward<Args>(traitsArgs)...)
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
         * @brief Resolve host by name as defined in either system hosts files or in A or in AAAA or in CNAME records.
         * @param context Task context. Task context is used as weak pointer in resolver and must be kept somewhere else.
         * @param callback Callback to invoke with query result.
         * @param hostName Name of the host.
         * @param port Port number to use in result endpoints.
         * @param ipVersion Version of IP protocol to use in queries and to filter result.
         */
        void resolveName(
            const common::TaskContextShared& context,
            ResolverCallback callback,
            const lib::string_view& hostName,
            uint16_t port=0,
            IpVersion ipVersion=IpVersion::ALL
        )
        {
            this->traits().resolveName(context,std::move(callback),hostName,port,ipVersion);
        }

        /**
         * @brief Resolve host by service as defined in SRV records.
         * @param context Task context. Task context is used as weak pointer in resolver and must be kept somewhere else.
         * @param callback Callback to invoke with query result.
         * @param name Full name of the service, e.g. _sip._tcp.example.com
         * @param ipVersion Version of IP protocol to use in query to resolve hostnames or filter addresses from SRV records.
         */
        void resolveService(
            const common::TaskContextShared& context,
            ResolverCallback callback,
            const lib::string_view& name,
            IpVersion ipVersion=IpVersion::ALL
        )
        {
            this->traits().resolveService(context,std::move(callback),name,ipVersion);
        }

        /**
         * @brief Resolve mx records.
         * @param context Task context. Task context is used as weak pointer in resolver and must be kept somewhere else.
         * @param callback Callback to invoke with query result.
         * @param name Full name of the service, e.g. _sip._tcp.example.com
         * @param ipVersion Version of IP protocol to use in query to resolve hostnames or filter addresses from SRV records.
         */
        void resolveMx(
            const common::TaskContextShared& context,
            ResolverCallback callback,
            const lib::string_view& name,
            IpVersion ipVersion=IpVersion::ALL
        )
        {
            this->traits().resolveMx(context,std::move(callback),name,ipVersion);
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

using ServiceName=common::StringOnStackT<256>;

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
        inline static ServiceName makeServiceName(
                const lib::string_view& domain,
                const lib::string_view& service,
                IpProtocol protocol
            )
        {
            ServiceName result;
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
         * @brief Resolve host by name as defined in either system hosts files or in A or in AAAA or in CNAME records.
         * @param hostName Name of the host.
         * @param callback Callback to invoke with query result.
         * @param context Task context. Task context is used as weak pointer in resolver and must be kept somewhere else.
         * @param port Port number to use in result endpoints.
         * @param ipVersion Version of IP protocol to use in queries and to filter result.
         */
        virtual void resolveName(
            const common::TaskContextShared& context,
            std::function<void (const common::Error&,std::vector<asio::IpEndpoint>)> callback,
            const lib::string_view& hostName,
            uint16_t port=0,
            IpVersion ipVersion=IpVersion::ALL
        )=0;

        /**
         * @brief Resolve host by service as defined in SRV records.
         * @param name Full name of the service, e.g. _sip._tcp.example.com
         * @param callback Callback to invoke with query result.
         * @param context Task context. Task context is used as weak pointer in resolver and must be kept somewhere else.
         * @param ipVersion Version of IP protocol to use in query to resolve hostnames or filter addresses from SRV records.
         */
        virtual void resolveService(            
            const common::TaskContextShared& context,
            std::function<void (const common::Error&,std::vector<asio::IpEndpoint>)> callback,
            const lib::string_view& name,
            IpVersion ipVersion=IpVersion::ALL
        ) =0;

        /**
         * @brief Resolve mx records.
         * @param name Full name of the service, e.g. _sip._tcp.example.com
         * @param callback Callback to invoke with query result.
         * @param context Task context. Task context is used as weak pointer in resolver and must be kept somewhere else.
         * @param ipVersion Version of IP protocol to use in query to resolve hostnames or filter addresses from SRV records.
         */
        virtual void resolveMx(            
            const common::TaskContextShared& context,
            std::function<void (const common::Error&,std::vector<asio::IpEndpoint>)> callback,
            const lib::string_view& name,
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
         * @brief Resolve host by name as defined in either system hosts files or in A or in AAAA or in CNAME records.
         * @param hostName Name of the host.
         * @param callback Callback to invoke with query result.
         * @param context Task context. Task context is used as weak pointer in resolver and must be kept somewhere else.
         * @param port Port number to use in result endpoints.
         * @param ipVersion Version of IP protocol to use in queries and to filter result.
         */
        virtual void resolveName(
            const common::TaskContextShared& context,
            std::function<void (const common::Error&,std::vector<asio::IpEndpoint>)> callback,
            const lib::string_view& hostName,
            uint16_t port=0,
            IpVersion ipVersion=IpVersion::ALL
        ) override
        {
            this->impl().resolveName(context,std::move(callback),hostName,port,ipVersion);
        }

        /**
         * @brief Resolve host by service as defined in SRV records.
         * @param name Full name of the service, e.g. _sip._tcp.example.com
         * @param callback Callback to invoke with query result.
         * @param context Task context. Task context is used as weak pointer in resolver and must be kept somewhere else.
         * @param ipVersion Version of IP protocol to use in query to resolve hostnames or filter addresses from SRV records.
         */
        virtual void resolveService(
            const common::TaskContextShared& context,
            std::function<void (const common::Error&,std::vector<asio::IpEndpoint>)> callback,
            const lib::string_view& name,
            IpVersion ipVersion=IpVersion::ALL
        ) override
        {
            this->impl().resolveService(context,std::move(callback),name,ipVersion);
        }

        /**
         * @brief Resolve mx records.
         * @param name Full name of the service, e.g. _sip._tcp.example.com
         * @param callback Callback to invoke with query result.
         * @param context Task context. Task context is used as weak pointer in resolver and must be kept somewhere else.
         * @param ipVersion Version of IP protocol to use in query to resolve hostnames or filter addresses from SRV records.
         */
        virtual void resolveMx(
            const common::TaskContextShared& context,
            std::function<void (const common::Error&,std::vector<asio::IpEndpoint>)> callback,
            const lib::string_view& name,
            IpVersion ipVersion=IpVersion::ALL
        ) override
        {
            this->impl().resolveMx(context,std::move(callback),name,ipVersion);
        }

        //! Cancel all pending queries
        virtual void cancel() override
        {
            this->impl().cancel();
        }
};

HATN_NETWORK_NAMESPACE_END

#endif // HATNNETWORKRESOLVER_H
