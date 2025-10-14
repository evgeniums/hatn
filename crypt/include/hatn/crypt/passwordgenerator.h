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

#include <hatn/common/error.h>
#include <hatn/common/memorylockeddata.h>

#include <hatn/crypt/crypt.h>

HATN_CRYPT_NAMESPACE_BEGIN

constexpr const char PasswordGenLetters[]="_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
constexpr const char PasswordGenDigits[]="0123456789";
constexpr const char PasswordGenSpecials[]="~!@#$%^&*(){}+=-:;<>,.|/?[]";

struct HATN_CRYPT_EXPORT PasswordGeneratorParameters
{
    constexpr static size_t DefaultMinLength=8;
    constexpr static size_t DefaultMaxLength=14;

    constexpr static size_t DefaultLettersWeight=20;
    constexpr static size_t DefaultDigitsWeight=4;
    constexpr static size_t DefaultSpecialsWeight=2;

    constexpr static bool DefaultHasSpecial=true;
    constexpr static bool DefaultHasDigit=true;

    size_t minLength=DefaultMinLength;
    size_t maxLength=DefaultMaxLength;

    size_t lettersWeight=DefaultLettersWeight;
    size_t digitsWeight=DefaultDigitsWeight;
    size_t specialsWeight=DefaultSpecialsWeight;

    bool hasSpecial=DefaultHasSpecial;
    bool hasDigit=DefaultHasDigit;

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

        inline Result<std::string> generateString()
        {
            common::MemoryLockedArray password;
            auto ec=generate(password);
            HATN_CHECK_EC(ec)

            return std::string{password.data(),password.size()};
        }

        void setDefaultParameters(PasswordGeneratorParameters params) noexcept
        {
            m_params=std::move(params);
        }

    private:

        virtual common::Error randBytes(char*,size_t);

        PasswordGeneratorParameters m_params;
};

HATN_CRYPT_NAMESPACE_END

#endif // HATNCRYPTPASSWORDGENERATOR_H
