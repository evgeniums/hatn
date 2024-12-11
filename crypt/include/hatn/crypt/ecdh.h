/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file crypt/ecdh.h
 *
 *      Base class for elliptic-curves based Diffie-Hellman processing
 *
 */
/****************************************************************************/

#ifndef HATNCRYPTECDH_H
#define HATNCRYPTECDH_H

#include <hatn/crypt/dh.h>

HATN_CRYPT_NAMESPACE_BEGIN

/**
 * @brief Base class for elliptic-curves based Diffie-Hellman processing
 */
class ECDH : public DiffieHellman
{
    public:

        /**
         * @brief Ctor
         * @param Cryptographic algorithm to use, must be of CryptAlgorithm::Type::ECDH
         */
        ECDH(const CryptAlgorithm* alg):m_alg(alg)
        {
            Assert(alg->type()==CryptAlgorithm::Type::ECDH,"Invalid type of algorithm");
        }

        //! Get cryptographic algorithm
        const CryptAlgorithm* alg() const noexcept
        {
            return m_alg;
        }

    private:

        const CryptAlgorithm* m_alg;
};

HATN_CRYPT_NAMESPACE_END

#endif // HATNCRYPTECDH_H
