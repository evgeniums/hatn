/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file crypt/passwordgenerator.cpp
 *
 * 	Password generator
 *
 */
/****************************************************************************/

#include <hatn/common/bytearray.h>
#include <hatn/common/random.h>

#include <hatn/crypt/passwordgenerator.h>

HATN_CRYPT_NAMESPACE_BEGIN
HATN_COMMON_USING

//---------------------------------------------------------------

common::Error PasswordGenerator::generate(MemoryLockedArray &password, const PasswordGeneratorParameters &params)
{
    password.clear();

    size_t allWeight=params.lettersWeight+params.digitsWeight+params.specialsWeight;
    double lettersWeight=static_cast<double>(params.lettersWeight)/static_cast<double>(allWeight)-std::numeric_limits<double>::epsilon();
    double digitsWeight=static_cast<double>(params.digitsWeight)/static_cast<double>(allWeight)-std::numeric_limits<double>::epsilon();
    double specialsWeight=static_cast<double>(params.specialsWeight)/static_cast<double>(allWeight)-std::numeric_limits<double>::epsilon();

    double maxUint=static_cast<double>(std::numeric_limits<uint32_t>::max())-std::numeric_limits<double>::epsilon();

    auto lettersRange=(params.lettersWeight==0) ? 0 : static_cast<uint32_t>(floor(lettersWeight*maxUint));
    auto digitsRange=(params.digitsWeight==0) ? 0 : (lettersRange + static_cast<uint32_t>(floor(digitsWeight*maxUint)));
    auto specialsRange=(params.specialsWeight==0) ? 0 : (digitsRange + static_cast<uint32_t>(floor(specialsWeight*maxUint)));
    std::ignore=specialsRange;

    std::vector<uint32_t,MemoryLockedDataAllocator<uint32_t>> rawData;
    constexpr size_t extraSize=20;
    rawData.resize(params.maxLength*2+4+extraSize);
    auto ec=randBytes(reinterpret_cast<char*>(rawData.data()),rawData.size()*sizeof(uint32_t));
    if (ec)
    {
        return ec;
    }

    auto length=params.maxLength;
    if (params.minLength<params.maxLength)
    {
        length=params.minLength+rawData[0]%(params.maxLength-params.minLength);
    }
    password.resize(length);

    bool hasSpecial=false;
    bool hasDigit=false;
    for (size_t i=0; i<length; i++)
    {
        const uint32_t& val=rawData[2*i+1];
        const uint32_t& val1=rawData[2*i+2];

        if (val<=lettersRange)
        {
            password[i]=PasswordGenLetters[val1%(sizeof(PasswordGenLetters)-1)];
        }
        else if (val<=digitsRange)
        {
            if (params.hasDigit)
            {
                password[i]=PasswordGenDigits[val1%(sizeof(PasswordGenDigits)-1)];
                hasDigit=true;
            }
            else
            {
                password[i]=PasswordGenLetters[val1%(sizeof(PasswordGenLetters)-1)];
            }
        }
        else
        {
            if (params.hasSpecial)
            {
                password[i]=PasswordGenSpecials[val1%(sizeof(PasswordGenSpecials)-1)];
                hasDigit=true;
            }
            else
            {
                password[i]=PasswordGenLetters[val1%(sizeof(PasswordGenLetters)-1)];
            }
        }
    }

    uint32_t* arr=rawData.data()+params.maxLength*2+4;
    lib::optional<uint32_t> lockIndex;
    if (!hasSpecial && params.hasSpecial && params.specialsWeight!=0)
    {
        uint32_t index=arr[0]%password.size();
        password[index]=PasswordGenSpecials[arr[1]%(sizeof(PasswordGenSpecials)-1)];
        lockIndex=index;
    }
    if (!hasDigit && params.hasDigit && params.digitsWeight!=0)
    {
        uint32_t index=arr[2]%password.size();
        if (lockIndex && index==lockIndex.value())
        {
            size_t i=4;
            while (index==lockIndex.value() && i<extraSize)
            {
                index=arr[i]%password.size();
                i++;
            }
            if (i==extraSize)
            {
                index=(lockIndex.value()+1)%password.size();
            }
        }
        password[index]=PasswordGenDigits[arr[3]%(sizeof(PasswordGenDigits)-1)];
    }

    return common::Error();
}

//---------------------------------------------------------------

common::Error PasswordGenerator::randBytes(char* buf,size_t size)
{
    common::Random::bytes(buf,size);
    return OK;
}

//---------------------------------------------------------------

HATN_CRYPT_NAMESPACE_END
