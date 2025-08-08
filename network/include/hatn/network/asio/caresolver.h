/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file network/asio/caresolver.h
  *
  *   DNS resolver that uses c-ares resolving library and ASIO events
  *
  */

/****************************************************************************/

#ifndef HATNCARESOLVER_H
#define HATNCARESOLVER_H

#include <hatn/common/pmr/allocatorfactory.h>

#include <hatn/network/network.h>
#include <hatn/network/resolver.h>

#define HATN_CARES_ERRORS(Do) \
    Do(CaresError,ARES_SUCCESS,_TR("OK")) \
    Do(CaresError,ARES_ENODATA,_TR("DNS server returned answer with no data","cares")) \
    Do(CaresError,ARES_EFORMERR,_TR("DNS server claims query was misformatted","cares")) \
    Do(CaresError,ARES_ESERVFAIL,_TR("DNS server returned general failure","cares")) \
    Do(CaresError,ARES_ENOTFOUND,_TR("Domain name not found","cares")) \
    Do(CaresError,ARES_ENOTIMP,_TR("DNS server does not implement requested operation","cares")) \
    Do(CaresError,ARES_EREFUSED,_TR("DNS server refused query","cares")) \
    Do(CaresError,ARES_EBADQUERY,_TR("Misformatted DNS query","cares")) \
    Do(CaresError,ARES_EBADNAME,_TR("Misformatted domain name","cares")) \
    Do(CaresError,ARES_EBADFAMILY,_TR("Unsupported address family","cares")) \
    Do(CaresError,ARES_EBADRESP,_TR("Misformatted DNS reply","cares")) \
    Do(CaresError,ARES_ECONNREFUSED,_TR("Could not contact DNS servers","cares")) \
    Do(CaresError,ARES_ETIMEOUT,_TR("Timeout while contacting DNS servers","cares")) \
    Do(CaresError,ARES_EOF,_TR("End of file","cares")) \
    Do(CaresError,ARES_EFILE,_TR("Error reading file","cares")) \
    Do(CaresError,ARES_ENOMEM,_TR("Out of memory","cares")) \
    Do(CaresError,ARES_EDESTRUCTION,_TR("Channel is being destroyed","cares")) \
    Do(CaresError,ARES_EBADSTR,_TR("Misformatted string","cares")) \
    Do(CaresError,ARES_EBADFLAGS,_TR("Illegal flags specified","cares")) \
    Do(CaresError,ARES_ENONAME,_TR("Given hostname is not numeric","cares")) \
    Do(CaresError,ARES_EBADHINTS,_TR("Illegal hints flags specified","cares")) \
    Do(CaresError,ARES_ENOTINITIALIZED,_TR("c-ares library initialization not yet performed","cares")) \
    Do(CaresError,ARES_ELOADIPHLPAPI,_TR("Error loading iphlpapi.dll","cares")) \
    Do(CaresError,ARES_EADDRGETNETWORKPARAMS,_TR("Could not find GetNetworkParams function","cares")) \
    Do(CaresError,ARES_ECANCELLED,_TR("DNS query cancelled","cares")) \
    Do(CaresError,ARES_ESERVICE,_TR("Invalid service name or number","cares")) \
    Do(CaresError,ARES_ENOSERVER,_TR("No DNS servers were configured","cares"))

HATN_NETWORK_NAMESPACE_BEGIN

//! Error codes of c-ares.
enum class CaresError : int
{
    HATN_CARES_ERRORS(HATN_ERROR_CODE)
};

//! c-ares errors codes as strings.
constexpr const char* const CaresErrorStrings[] = {
    HATN_CARES_ERRORS(HATN_ERROR_STR)
};

//! c-ares error code to string.
inline const char* caresErrorString(CaresError code)
{
    return errorString(code,CaresErrorStrings);
}

//! Error category of c-ares library
class HATN_NETWORK_EXPORT CaresErrorCategory final : public common::ErrorCategory
{
    public:

        //! Name of the category.
        virtual const char *name() const noexcept override
        {
            return "hatn.network.cares";
        }

        //! Get description for the code
        virtual std::string message(int code) const override;

        //! Get string representation of the code.
        virtual const char* codeString(int code) const override;

        //! Get category.
        static const CaresErrorCategory& getCategory() noexcept;
};

//! Make Error form c-ares error code
inline Error caresError(int code) noexcept
{
    return Error(code,&CaresErrorCategory::getCategory());
}

namespace asio {

class CaResolver;
class CaResolverTraits_p;

//! DNS resolver that uses c-ares resolving library ans ASIO events
class HATN_NETWORK_EXPORT CaResolverTraits
{
    public:

        using Callback=ResolverCallback;

        /**
         * @brief Constructor
         * @param thread Thread to work in
         * @param nameServers Override system's name servers
         * @param resolvConfPath Override system's resolv.conf or similar file
         *
         * @throws ErrorException if initialization fails
         */
        CaResolverTraits(
            CaResolver* resolver,
            const std::vector<NameServer>& nameServers,
            const std::string& resolvConfPath=std::string()
        );

        ~CaResolverTraits()=default;
        CaResolverTraits(const CaResolverTraits&)=delete;
        CaResolverTraits(CaResolverTraits&&) =default;
        CaResolverTraits& operator=(const CaResolverTraits&)=delete;
        CaResolverTraits& operator=(CaResolverTraits&&) =default;

        //! Enable using local hosts file (default true)
        void setUseLocalHostsFile(bool enable) noexcept;

        //! Check if local hosts file used (default true)
        bool isUseLocalHostsFile() const noexcept;

        void resolveName(
            const common::TaskContextShared& context,
            Callback callback,
            const lib::string_view& hostName,
            uint16_t port,
            IpVersion ipVersion
        );
        void resolveService(
            const common::TaskContextShared& context,
            Callback callback,
            const lib::string_view& name,
            IpVersion ipVersion
        );
        void resolveMx(            
            const common::TaskContextShared& context,
            Callback callback,
            const lib::string_view& name,
            IpVersion ipVersion
        );
        void cancel();
        void cleanup();

    private:

        std::shared_ptr<CaResolverTraits_p> d;
};

//! DNS resolver that uses c-ares resolving library and ASIO events
class CaResolver : public Resolver<CaResolverTraits>
{
    public:

        /**
         * @brief Constructor
         * @param thread Thread to work in
         * @param nameServers Override system's name servers
         * @param resolvConfPath Override system's resolv.conf or similar file
         *
         * @throws ErrorException if initialization fails
         */
        CaResolver(
            common::Thread* thread,
            const std::vector<NameServer>& nameServers,
            const std::string& resolvConfPath=std::string()
        ) : Resolver<CaResolverTraits>(
                thread,
                this,
                nameServers,
                resolvConfPath
            )
        {}

        /**
         * @brief Overloaded constructor, see full constructor above
         */
        explicit CaResolver(
            const std::vector<NameServer>& nameServers
        )  : CaResolver(common::Thread::currentThread(),nameServers)
        {}

        /**
         * @brief Overloaded constructor, see full constructor above
         */
        CaResolver(
            common::Thread* thread,
            const std::string& resolvConfPath
        ) : CaResolver(thread,std::vector<NameServer>(),resolvConfPath)
        {}

        /**
         * @brief Overloaded constructor, see full constructor above
         */
        explicit CaResolver(
            common::Thread* thread
        ) : CaResolver(thread,std::vector<NameServer>())
        {}

        /**
         * @brief Overloaded constructor, see full constructor above
         */
        CaResolver() : CaResolver(common::Thread::currentThread(),std::vector<NameServer>(),std::string())
        {}

        //! Enable using local hosts file (default true)
        inline void setUseLocalHostsFile(bool enable) noexcept
        {
            traits().setUseLocalHostsFile(enable);
        }

        //! Check if local hosts file used (default true)
        inline bool isUseLocalHostsFile() const noexcept
        {
            return traits().isUseLocalHostsFile();
        }
};

//! Polymotphic version of DNS resolver that uses c-ares resolving library ans ASIO events
class CaResolverV : public ResolverTmlV<CaResolver>
{
    public:

        using ResolverTmlV<CaResolver>::ResolverTmlV;

        //! Enable using local hosts file (default true)
        inline void setUseLocalHostsFile(bool enable) noexcept
        {
            this->impl().setUseLocalHostsFile(enable);
        }

        //! Check if local hosts file used (default true)
        inline bool isUseLocalHostsFile() const noexcept
        {
            return this->impl().isUseLocalHostsFile();
        }
};

} // namespace asio

HATN_NETWORK_NAMESPACE_END

#endif // HATNCARESOLVER_H
