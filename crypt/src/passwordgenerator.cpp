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

#include <hatn/crypt/passwordgenerator.h>

namespace hatn {

using namespace common;

namespace crypt {

static const char Letters[]="_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
static const char Digits[]="0123456789";
static const char Specials[]="~!@#$%^&*(){}+=-:;<>,.|/?";

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
    rawData.resize(params.maxLength*2+1);
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

    for (size_t i=0; i<length; i++)
    {
        const uint32_t& val=rawData[2*i+1];
        const uint32_t& val1=rawData[2*i+2];

        if (val<=lettersRange)
        {
            password[i]=Letters[val1%(sizeof(Letters)-1)];
        }
        else if (val<=digitsRange)
        {
            password[i]=Digits[val1%(sizeof(Digits)-1)];
        }
        else
        {
            password[i]=Specials[val1%(sizeof(Specials)-1)];
        }
    }

    return common::Error();
}

//---------------------------------------------------------------
HATN_CRYPT_NAMESPACE_END
