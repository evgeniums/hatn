/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*

*/
/** @file clientserver/models/accountconfigformat.сpp
  *
  */

#include <hatn/dataunit/visitors.h>
#include <hatn/dataunit/wirebufsolid.h>

#include <hatn/crypt/cryptcontainerheader.h>

#include <hatn/clientserver/clientserver.h>
#include <hatn/clientserver/models/accountconfigformat.h>

#include <hatn/dataunit/ipp/syntax.ipp>
#include <hatn/dataunit/ipp/wirebuf.ipp>
#include <hatn/dataunit/ipp/objectid.ipp>

HATN_CLIENT_SERVER_NAMESPACE_BEGIN

namespace {

//! account_config_token::content below this length cannot be a real payload of either shape --
//! the smallest possible CryptContainerHeader is 22 bytes, and an account_config with all four
//! required fields set cannot serialize any shorter. Matches the todo's own note that a real
//! account config is not tiny, mirroring shared_invitation's few-hundred-bytes expectation.
constexpr const size_t MinContentSize=16;

}

//--------------------------------------------------------------------------

AccountConfigInfo classifyAccountConfigT::operator () (
        const char* data,
        size_t size,
        size_t maxSize
    ) const
{
    AccountConfigInfo info;

    if (data==nullptr || size==0 || size>maxSize)
    {
        return info;
    }

    // Envelope only -- exactly like classifySharedInvitationT, never touches anything beyond
    // what is needed to decide isAccountConfig/encrypted/expired.
    account_config_token::type token;
    Error ec;
    if (!du::io::deserializeInline(token,data,size,ec))
    {
        // Not an account config, not an error: classification is a question, see this file's
        // own doc comment (accountconfigformat.h).
        return info;
    }

    const auto& contentField=token.field(account_config_token::content);
    if (!contentField.isSet())
    {
        return info;
    }
    auto content=token.fieldValue(account_config_token::content);
    if (content.size()<MinContentSize)
    {
        return info;
    }

    info.encrypted=token.fieldValue(account_config_token::encrypted);

    if (info.encrypted)
    {
        // content is a crypt::CryptContainer::pack() output -- unlike the plaintext branch
        // below, this DOES carry a real magic (CryptContainerHeader::PREFIX/STREAM_PREFIX plus
        // a version byte), checked here with no decryption and no passphrase. Everything past
        // the header is unknowable without the passphrase, so expired stays false: the caller
        // routes this to the passphrase-prompt flow, which already handles expiry once decrypted.
        if (content.size()<crypt::CryptContainerHeader::size())
        {
            return info;
        }
        crypt::CryptContainerHeader header(content.data());
        if (!header.checkPrefix() || header.version()!=crypt::CryptContainerHeader::VERSION)
        {
            return info;
        }
        info.isAccountConfig=true;
        return info;
    }

    // Plaintext: account_config_token carries no magic of its own here, so parse success alone
    // is not a discriminator (hatn dataunit deserialization skips unknown tags -- an unrelated
    // blob can deserialize into an all-unset account_config_token/account_config). Require the
    // four fields activateaccountconfig.cpp itself hard-needs to activate a config
    // (server_route check at :167, token/config_id/account_id read unconditionally at
    // :232,312-314) -- a blob missing any of them cannot honestly be called an account config.
    account_config::type config;
    if (!du::io::deserializeInline(config,content,ec))
    {
        return info;
    }

    if (!config.field(account_config::config_id).isSet())
    {
        return info;
    }
    if (!config.field(account_config::account_id).isSet())
    {
        return info;
    }
    if (config.field(account_config::server_route).count()==0)
    {
        return info;
    }
    const auto& tokenField=config.field(account_config::token);
    if (!tokenField.isSet() || config.fieldValue(account_config::token).empty())
    {
        return info;
    }

    info.isAccountConfig=true;

    const auto& validTillField=config.field(account_config::valid_till);
    if (validTillField.isSet())
    {
        auto now=common::DateTime::currentUtc();
        info.expired=now.after(validTillField.value());
    }

    return info;
}

//--------------------------------------------------------------------------

HATN_CLIENT_SERVER_NAMESPACE_END
