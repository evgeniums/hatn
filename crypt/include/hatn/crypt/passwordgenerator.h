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

constexpr const char PasswordGenLetters[]="_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
constexpr const char PasswordGenDigits[]="0123456789";
constexpr const char PasswordGenSpecials[]="~!@#$%^&*(){}+=-:;<>,.|/?[]";

struct HATN_CRYPT_EXPORT  PasswordGeneratorParameters
{
    size_t minLength=8;
    size_t maxLength=14;

    size_t lettersWeight=20;
    size_t digitsWeight=4;
    size_t specialsWeight=2;

    bool hasSpecial=true;
    bool hasDigit=true;

    //! @todo Implement configurable source arrays
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
