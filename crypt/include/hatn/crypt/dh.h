/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file crypt/dh.h
 *
 *      Base class for generation/derivation of Diffie-Hellmann key.
 *
 */
/****************************************************************************/

#ifndef HATNCRYPTDH_H
#define HATNCRYPTDH_H

#include <hatn/crypt/crypt.h>
#include <hatn/crypt/diffiehellman.h>
#include <hatn/crypt/dhlegacy.h>

HATN_CRYPT_NAMESPACE_BEGIN

#ifndef HATN_CRYPT_LEGACY_DH

/**
 * @brief Base class for generation/derivation of Diffie-Hellmann key.
 *
 * Currently DH can be used only with predefined safe prime groups.
 * Generating and importing custom DH parameters not supported.
 *
 * Predefined safe prime groups must be implemented by a crypt plugin and bound to DH algorithms
 * that can be obtained with CryptPlugin::listDHs(). Name of algorithm starts with name of corresponding group,
 * e.g for Openssl 3 backend CryptPlugin::listDHs() would return the list containing
 * "ffdhe2048", "ffdhe3072", "ffdhe4096", "ffdhe6144", "ffdhe8192", "modp_2048", "modp_3072",
 * "modp_4096", "modp_6144", "modp_8192".
 */
class HATN_CRYPT_EXPORT DH : public DiffieHellman
{
    public:

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
        common::Error init(const CryptAlgorithm* alg=nullptr)
        {
            if (alg!=nullptr)
            {
                m_alg.ptr=alg;
            }
            if (m_alg.isNull() || !m_alg->isType(CryptAlgorithm::Type::DH))
            {
                return cryptError(CryptError::INVALID_ALGORITHM);
            }
            return OK;
        }

    private:

        common::ConstPointerWithInit<CryptAlgorithm> m_alg;
};

#endif

HATN_CRYPT_NAMESPACE_END

#endif // HATNCRYPTDH_H
