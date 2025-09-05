/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*

*/
/** @file clientserver/accountconfigparser.сpp
  *
  */

#include <hatn/dataunit/visitors.h>
#include <hatn/dataunit/wirebufsolid.h>

#include <hatn/crypt/ciphersuite.h>
#include <hatn/crypt/cryptcontainer.h>

#include <hatn/clientserver/clientserver.h>
#include <hatn/clientserver/clientservererror.h>
#include <hatn/clientserver/accountconfigparser.h>

#include <hatn/dataunit/ipp/syntax.ipp>
#include <hatn/dataunit/ipp/wirebuf.ipp>
#include <hatn/dataunit/ipp/objectid.ipp>

HATN_CLIENT_SERVER_NAMESPACE_BEGIN

//--------------------------------------------------------------------------

Result<common::SharedPtr<account_config::managed>> parseAccountConfigT::operator () (
        const crypt::CipherSuites* cipherSuites,
        lib::string_view data,
        lib::string_view password
    ) const
{
    Error ec;

    // deserialize token
    account_config_token::type token;
    auto ok=du::io::deserializeInline(token,data,ec);
    if (!ok)
    {
        return common::chainErrors(std::move(ec),clientServerError(ClientServerError::ACCOUNT_CONFIG_DESERIALIZATION));
    }

    lib::string_view configData=token.fieldValue(account_config_token::content);

    // decrypt content if required
    std::string decrypted;
    if (token.fieldValue(account_config_token::encrypted))
    {
        if (password.empty())
        {
            return clientServerError(ClientServerError::ACCOUNT_CONFIG_PASSPHRASE_REQUIRED);
        }

        crypt::CryptContainer decryptor;
        decryptor.setCipherSuites(cipherSuites);
        decryptor.setPassphrase(password);
        ec=decryptor.unpack(configData,decrypted);
        if (ec)
        {
            return clientServerError(ClientServerError::ACCOUNT_CONFIG_DECRYPTION);
        }

        configData=decrypted;
    }

    // deserialize account_config
    auto accountConfig=common::makeShared<account_config::managed>();
    if (!du::io::deserializeInline(*accountConfig,configData,ec))
    {
        return common::chainErrors(std::move(ec),clientServerError(ClientServerError::ACCOUNT_CONFIG_DATA_DESERIALIZATION));
    }

#if 0
    // print debug
    std::cout << "Config: " << accountConfig->toString(true) << std::endl;
#endif

    // done
    return accountConfig;
}

//--------------------------------------------------------------------------

Error checkAccountConfigT::operator () (
        const crypt::CipherSuites* cipherSuites,
        const account_config::managed* accountConfig
    ) const
{
    std::ignore=cipherSuites;

    if (accountConfig->field(account_config::valid_till).fieldIsSet())
    {
        auto now=common::DateTime::currentUtc();
        if (now.after(accountConfig->field(account_config::valid_till).value()))
        {
            return clientServerError(ClientServerError::ACCOUNT_CONFIG_EXPIRED);
        }
    }

    //! @todo check certificate chain

    //! @todo check signature

    // done
    return OK;
}

//--------------------------------------------------------------------------

HATN_CLIENT_SERVER_NAMESPACE_END
