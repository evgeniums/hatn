/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file crypt/dhlegacy.h
 *
 *      Legacy DH types.
 */
/****************************************************************************/

#ifndef HATNCRYPTDHLEGACY_H
#define HATNCRYPTDHLEGACY_H

#include <memory>

#include <hatn/common/bytearray.h>
#include <hatn/common/fixedbytearray.h>

#include <hatn/crypt/crypt.h>
#include <hatn/crypt/keycontainer.h>
#include <hatn/crypt/diffiehellman.h>

HATN_CRYPT_NAMESPACE_BEGIN

#ifdef HATN_CRYPT_LEGACY_DH

/**
 * @brief Base class for processor of Diffie-Hellmann key generation/derivation.
 *
 * Class DH inherits from KeyContainer because it itself contains DH parameters.
 *
 * Those parameters must be pre-generated somewhere. Generation of DH parameters is a time consuming operation
 * and must not be performed in production in runtime, so the parameters generation is out of scope here.
 * For example. you can use predefined MODP groups specified in RFC3526.
 *
 * DH parameters must be imported to the DH object on initialization of the object before use.
 * Usually, parameters are stored in PEM format. For example,
 * in OpenSSL backend they can be imported from the buffer into native handler with PEM_read_bio_DHparams().
 * PEM-encoded MODP groups of RFC3526 can be found on https://wiki.openssl.org/index.php/Diffie-Hellman_parameters
 * (see DHRfc3526 below).
 */
class HATN_CRYPT_EXPORT DH : public DiffieHellman,
           public KeyContainer<common::ByteArray>
{
    public:

        using KeyContainer<common::ByteArray>::KeyContainer;

        /**
         * @brief Import parameters from buffer
         * @param buf Buffer to import from
         * @param size Size of the buffer
         * @param format Data format
         * @param keepContent Keep imported content
         * @return Operation status
         */
        virtual common::Error importParamsFromBuf(
            const char* buf,
            size_t size,
            ContainerFormat format=ContainerFormat::PEM,
            bool keepContent=true
        ) =0;

        /**
         * @brief Import parameters from buffer
         * @param container Container to import from
         * @param format Data format
         * @param keepContent Keep imported content
         * @return Operation status
         */
        template <typename ContainerT>
        common::Error importParamsFromBuf(
            const ContainerT& container,
            ContainerFormat format=ContainerFormat::PEM,
            bool keepContent=true
        )
        {
            return importParamsFromBuf(container.data(),container.size(),format,keepContent);
        }

        /**
         * @brief Import parameters from global DH storage
         * @param name Name of parameters
         * @param sha1 SHA1 of Parameters
         * @param keepContent Keep imported content
         * @return Operation status
         */
        common::Error importParamsFromStorage(
            const std::string& name,
            const std::string& sha1=std::string(),
            bool keepContent=true
        );

        /**
         * @brief Import parameters from self content
         * @return Operation status
         */
        common::Error autoImportParams(
        )
        {
            return importParamsFromBuf(content().data(),content().size(),format(),false);
        }

        /**
         * @brief Import parameters from file
         * @param container Container to import from
         * @param format Data format
         * @return Operation status
         */
        common::Error importParamsFromFile(
            const std::string& fileName,
            ContainerFormat format=ContainerFormat::PEM,
            bool keepContent=true
        )
        {
            HATN_CHECK_RETURN(loadFromFile(fileName))
            setFormat(format);
            HATN_CHECK_RETURN(autoImportParams());
            if (!keepContent)
            {
                content().clear();
            }
            return common::Error();
        }

        void setAlg(const CryptAlgorithm* alg) noexcept
        {
            m_alg.ptr=alg;
        }

        const CryptAlgorithm* alg() const noexcept
        {
            return m_alg.ptr;
        }

        /**
         * @brief Initialize processor with algorithm
         * @param alg Algorithm, if nullptr then m_alg will be used
         * @return Operation status
         */
        common::Error init(const CryptAlgorithm* alg=nullptr);

    private:

        common::ConstPointerWithInit<CryptAlgorithm*> m_alg;
};

//! Groups of DH parameters as defined in RFC3526
struct HATN_CRYPT_EXPORT DHRfc3526
{
    static common::ByteArray group1024() noexcept;
    static common::ByteArray group1536() noexcept;
    static common::ByteArray group2048() noexcept;
    static common::ByteArray group3072() noexcept;
    static common::ByteArray group4096() noexcept;

    static std::string group1024Name();
    static std::string group1536Name();
    static std::string group2048Name();
    static std::string group3072Name();
    static std::string group4096Name();
};

//! Storage of DH parameters
class HATN_CRYPT_EXPORT DHParamsStorage final : public common::Singleton
{
    public:

        HATN_SINGLETON_DECLARE()

        enum class AlgParam : int
        {
            Name=0,
            Sha1=1
        };

        struct Params
        {
            common::ByteArray data;
            std::string name;
            std::string sha1;
            ContainerFormat format;

            Params(
                common::ByteArray data,
                std::string name,
                std::string sha1,
                ContainerFormat format
            ) : data(std::move(data)),
                name(std::move(name)),
                sha1(std::move(sha1)),
                format(format)
            {}
        };

        //! Get singltone's instance
        static DHParamsStorage* instance();
        //! Destroy singleton's instance
        static void free();

        /**
         * @brief Add parameters to storage
         * @param params Parameters
         */
        void addParams(const std::shared_ptr<Params>& params);
        /**
         * @brief Add parameters from data container
         * @param name Parameters' name
         * @param content Content
         * @param format Format of content
         */
        void addParams(std::string name, common::ByteArray content,ContainerFormat format=ContainerFormat::PEM);
        /**
         * @brief Add parameters from file
         * @param name Parameters' name
         * @param fileName File name
         * @param format Format of content
         *
         * @throws common::ErrorException if failed to load data from file
         */
        void addParams(std::string name, const std::string& fileName,ContainerFormat format=ContainerFormat::PEM);

        /**
         * @brief Find parameters by name
         * @param name Name of parameters
         * @return Found parameters or empty shared_ptr
         */
        std::shared_ptr<Params> findParamsByName(const std::string& name) const noexcept;
        /**
         * @brief Find parameters by sha1
         * @param sha1 Sha1 on raw content (in case of PEM the content is decoded from base64 first, then sha1 is calculated)
         * @return Found parameters or empty shared_ptr
         */
        std::shared_ptr<Params> findParamsBySha1(const std::string& sha1) const noexcept;

        /**
         * @brief Remove parameters by name
         * @param name Name of parameters
         */
        void removeParamsByName(const std::string& name);

        /**
         * @brief Remove parameters by sha1
         * @param sha1 Sha1 on raw content (in case of PEM the content is decoded from base64 first, then sha1 is calculated)
         */
        void removeParamsBySha1(const std::string& sha1);

    private:

        std::map<std::string,std::shared_ptr<Params>> m_paramsByName;
        std::map<std::string,std::shared_ptr<Params>> m_paramsBySha1;
};

#endif

HATN_CRYPT_NAMESPACE_END

#endif // HATNCRYPTDHLEGACY_H
