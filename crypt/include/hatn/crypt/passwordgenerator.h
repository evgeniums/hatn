/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file crypt/passwordgenerator.h
 *
 *  Password generator
 *
 */
/****************************************************************************/

#ifndef HATNCRYPTPASSWORDGENERATOR_H
#define HATNCRYPTPASSWORDGENERATOR_H

#include <functional>

#include <hatn/common/error.h>
#include <hatn/common/memorylockeddata.h>

#include <hatn/crypt/crypt.h>

HATN_CRYPT_NAMESPACE_BEGIN

struct HATN_CRYPT_EXPORT  PasswordGeneratorParameters
{
    size_t minLength=8;
    size_t maxLength=12;

    size_t lettersWeight=8;
    size_t digitsWeight=2;
    size_t specialsWeight=1;
};

/**
 * @brief The PasswordGenerator class
 */
class HATN_CRYPT_EXPORT PasswordGenerator
{
    public:

        PasswordGenerator()=default;
        PasswordGenerator(PasswordGeneratorParameters params) noexcept : m_params(std::move(params))
        {}

        virtual ~PasswordGenerator()=default;
        PasswordGenerator(const PasswordGenerator&)=delete;
        PasswordGenerator(PasswordGenerator&&) =delete;
        PasswordGenerator& operator=(const PasswordGenerator&)=delete;
        PasswordGenerator& operator=(PasswordGenerator&&) =delete;

        virtual common::Error generate(common::MemoryLockedArray& password,
                                       const PasswordGeneratorParameters& params
                                    );

        inline common::Error generate(common::MemoryLockedArray& password)
        {
            return generate(password,m_params);
        }

        void setDefaultParameters(PasswordGeneratorParameters params) noexcept
        {
            m_params=std::move(params);
        }

    private:

        virtual common::Error randBytes(char*,size_t) =0;

        PasswordGeneratorParameters m_params;
};

HATN_CRYPT_NAMESPACE_END

#endif // HATNCRYPTPASSWORDGENERATOR_H
