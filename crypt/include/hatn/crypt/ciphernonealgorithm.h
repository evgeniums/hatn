/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file crypt/ciphernonealgotithm.h
  *
  *   Cipher algorithm that just copies data without ectual encryption/decription
  *
  */

/****************************************************************************/

#ifndef HATNCIPHERNONEALG_H
#define HATNCIPHERNONEALG_H

#include <hatn/crypt/crypt.h>
#include <hatn/crypt/cryptalgorithm.h>
#include <hatn/crypt/securekey.h>

#include <hatn/common/makeshared.h>

HATN_CRYPT_NAMESPACE_BEGIN

struct CipherNoneAlgorithmStub
{
    static CipherNoneAlgorithmStub& stub() noexcept
    {
        static CipherNoneAlgorithmStub inst;
        return inst;
    }
};

//! General descriptor of cryptigraphic algorithm
class CipherNoneAlgorithm : public CryptAlgorithm
{
    public:

        /**
         * @brief Ctor
         * @param engine Backend crypt engine
         * @param type Algorithm type
         * @param name Name of algorithm encoding actual name of cryptographic algorithm in backend and optional set of algorithm parameters.
         */
        CipherNoneAlgorithm(const CryptEngine* engine,const char* name) noexcept
            : CryptAlgorithm(engine,CryptAlgorithm::Type::SENCRYPTION,name,0,&CipherNoneAlgorithmStub::stub())
        {}

        //! Get key size of algorithm if applicable
        virtual size_t keySize() const override
        {
            return 0;
        }

        //! Get IV size of algorithm if applicable
        virtual size_t ivSize() const override
        {
            return 0;
        }

        //! Check if padding is enabled if applicable
        virtual bool enablePadding() const override
        {
            return false;
        }
        virtual void setEnablePadding(bool enable) override
        {
            std::ignore=enable;
        }

        //! Get block size of algorithm if applicable
        virtual size_t blockSize() const override
        {
            return 1;
        }

        virtual common::SharedPtr<SymmetricKey> createSymmetricKey() const override
        {
            auto key=common::makeShared<SymmetricKey>();
            key->setAlg(this);
            return key;
        }

        virtual bool isBackendAlgorithm() const override
        {
            return false;
        }

        virtual bool isNone() const noexcept override
        {
            return true;
        }
};

HATN_CRYPT_NAMESPACE_END

#endif // HATNCIPHERNONEALG_H
