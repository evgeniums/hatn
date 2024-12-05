/*
   Copyright (c) 2019 - current, Evgeny Sidorov (esid1976@gmail.com), All rights reserved
    
    
  */

/****************************************************************************/
/*
    
*/
/** \file network/asio/caresolver.h
  *
  *   DNS resolver that uses c-ares resolving library ans ASIO events
  *
  */

/****************************************************************************/

#ifndef HATNCARESOLVER_H
#define HATNCARESOLVER_H

#include <hatn/common/pmr/allocatorfactory.h>

#include <hatn/network/network.h>
#include <hatn/network/resolver.h>

namespace hatn {
namespace network {

//! c-ares library initialization and cleanup
class HATN_NETWORK_EXPORT CaresLib
{
    public:

        CaresLib()=delete;
        ~CaresLib()=delete;
        CaresLib(const CaresLib&)=delete;
        CaresLib(CaresLib&&) =delete;
        CaresLib& operator=(const CaresLib&)=delete;
        CaresLib& operator=(CaresLib&&) =delete;

        //! Initialize library
        static common::Error init(
            common::pmr::AllocatorFactory* allocatorFactory=nullptr
        );

        //! Cleanup library
        static void cleanup();

        //! Get allocator factory to use in resolver
        inline static common::pmr::AllocatorFactory* allocatorFactory() noexcept
        {
            return m_allocatorFactory;
        }

    private:

        static common::pmr::AllocatorFactory *m_allocatorFactory;
};


//! Errors of c-ares library
class HATN_NETWORK_EXPORT CaresErrorCategory final : public common::ErrorCategory
{
    public:

        //! Name of the category
        virtual const char *name() const noexcept override
        {
            return "hatn.network.cares";
        }

        //! Get description for the code
        virtual std::string message(int code) const override;

        //! Get category
        static const CaresErrorCategory& getCategory() noexcept;

        //! Get string representation of the code.
        virtual const char* codeString(int code) const
        {
            //! @todo Implement
            std::ignore=code;
            return nullptr;
        }
};

//! Make Error form c-ares error code
inline common::Error makeCaresError(int code) noexcept
{
    return ::hatn::common::Error(code,&CaresErrorCategory::getCategory());
}

namespace asio {

class CaResolver;
class CaResolverTraits_p;

//! DNS resolver that uses c-ares resolving library ans ASIO events
class HATN_NETWORK_EXPORT CaResolverTraits
{
    public:

        /**
         * @brief Constructor
         * @param thread Thread to work in
         * @param nameServers Override system's name servers
         * @param resolvConfPath Override system's resolv.conf or similar file
         *
         * @throws common::ErrorException if initialization fails
         */
        CaResolverTraits(
            CaResolver* resolver,
            common::Thread* thread,
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
            const char* hostName,
            std::function<void (const common::Error&,std::vector<asio::IpEndpoint>)> callback,
            uint16_t port,
            IpVersion ipVersion
        );
        void resolveService(
            const char* name,
            std::function<void (const common::Error&,std::vector<asio::IpEndpoint>)> callback,
            IpVersion ipVersion
        );
        void resolveMx(
            const char* name,
            std::function<void (const common::Error&,std::vector<asio::IpEndpoint>)> callback,
            IpVersion ipVersion
        );
        void cancel();
        void cleanup();

        const char* idStr() const noexcept;

    private:

        std::shared_ptr<CaResolverTraits_p> d;
};

//! DNS resolver that uses c-ares resolving library ans ASIO events
class CaResolver : public Resolver<CaResolverTraits>
{
    public:

        /**
         * @brief Constructor
         * @param thread Thread to work in
         * @param nameServers Override system's name servers
         * @param resolvConfPath Override system's resolv.conf or similar file
         * @param id ID of this resolver
         *
         * @throws common::ErrorException if initialization fails
         */
        CaResolver(
            common::Thread* thread,
            const std::vector<NameServer>& nameServers,
            const std::string& resolvConfPath=std::string(),
            common::STR_ID_TYPE id=common::STR_ID_TYPE()
        ) : Resolver<CaResolverTraits>(
                thread,
                std::move(id),
                this,
                thread,
                nameServers,
                resolvConfPath
            )
        {}

        /**
         * @brief Overloaded constructor, see full constructor above
         */
        CaResolver(
            const std::vector<NameServer>& nameServers,
            common::STR_ID_TYPE id=common::STR_ID_TYPE()
        )  : CaResolver(common::Thread::currentThread(),nameServers,std::string(),std::move(id))
        {}

        /**
         * @brief Overloaded constructor, see full constructor above
         */
        CaResolver(
            common::Thread* thread,
            const std::string& resolvConfPath,
            common::STR_ID_TYPE id=common::STR_ID_TYPE()
        ) : CaResolver(thread,std::vector<NameServer>(),resolvConfPath,std::move(id))
        {}

        /**
         * @brief Overloaded constructor, see full constructor above
         */
        CaResolver(
            common::Thread* thread,
            common::STR_ID_TYPE id=common::STR_ID_TYPE()
        ) : CaResolver(thread,std::vector<NameServer>(),std::string(),std::move(id))
        {}

        /**
         * @brief Overloaded constructor, see full constructor above
         */
        CaResolver(
            common::STR_ID_TYPE id=common::STR_ID_TYPE()
        ) : CaResolver(common::Thread::currentThread(),std::vector<NameServer>(),std::string(),std::move(id))
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
} // namespace network
} // namespace hatn

#endif // HATNCARESOLVER_H
