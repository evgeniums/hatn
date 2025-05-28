/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*
    
*/
/** @file clientserver/accountconfigparser.h
  */

/****************************************************************************/

#ifndef HATNACCOUNTCONFIGPARSER_H
#define HATNACCOUNTCONFIGPARSER_H

#include <hatn/crypt/crypt.h>

#include <hatn/clientserver/clientserver.h>
#include <hatn/clientserver/models/accountconfig.h>

HATN_CRYPT_NAMESPACE_BEGIN

class CipherSuites;

HATN_CRYPT_NAMESPACE_END

HATN_CLIENT_SERVER_NAMESPACE_BEGIN

class parseAccountConfigT
{
    public:

        Result<common::SharedPtr<account_config::managed>> operator () (
            const crypt::CipherSuites* cipherSuites,
            lib::string_view data,
            lib::string_view password={}
        ) const;
};
constexpr parseAccountConfigT parseAccountConfig{};

HATN_CLIENT_SERVER_NAMESPACE_END

#endif // HATNACCOUNTCONFIGPARSER_H
