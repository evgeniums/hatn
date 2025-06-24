/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*

*/
/** @file serverapp/sharedsecretprotocol.сpp
  *
  */

#include <hatn/crypt/cryptcontainer.h>

#include <hatn/dataunit/visitors.h>
#include <hatn/dataunit/wirebufsolid.h>

#include <hatn/serverapp/auth/authtokens.h>
#include <hatn/serverapp/auth/sharedsecretprotocol.h>

#include <hatn/dataunit/ipp/syntax.ipp>
#include <hatn/dataunit/ipp/objectid.ipp>
#include <hatn/dataunit/ipp/wirebuf.ipp>

HATN_SERVERAPP_NAMESPACE_BEGIN

//--------------------------------------------------------------------------

Error SharedSecretAuthBase::init(const crypt::CipherSuite* suite)
{
    //! @todo use validator for config
    if (config().fieldValue(auth_protocol_shared_secret_config::secret).empty())
    {
        return common::genericError(_TR("Encryption secret for shared secret authentication must be not empty"));
    }

    m_suite=suite;
    Error ec;
    auto passphraseKey=suite->createPassphraseKey(ec);
    HATN_CHECK_EC(ec)
    passphraseKey->set(config().fieldValue(auth_protocol_shared_secret_config::secret));
    return OK;
}

//--------------------------------------------------------------------------

Result<common::SharedPtr<auth_negotiate_response::managed>> SharedSecretAuthBase::prepareChallengeToken(
        const common::SharedPtr<auth_negotiate_request::managed>& authRequest,
        const common::pmr::AllocatorFactory* factory
    )
{
    // prepare response
    auto response=factory->createObject<auth_negotiate_response::managed>();

    auto* proto=response->field(auth_negotiate_response::protocol).mutableValue();
    proto->setFieldValue(HATN_API_NAMESPACE::auth_protocol::protocol,name());
    proto->setFieldValue(HATN_API_NAMESPACE::auth_protocol::version,version());

    // generate challenge

    auto randGen=m_suite->suites()->randomGenerator();
    if (randGen==nullptr)
    {
        return crypt::cryptError(crypt::CryptError::RANDOM_GENERATOR_NOT_DEFINED);
    }

    auto challenge=factory->createObject<auth_hss_challenge::managed>();
    auto& challengeField=challenge->field(auth_hss_challenge::challenge);
    auto* challengeBuf=challengeField.buf(true);
    auto ec=randGen->randContainer(*challengeBuf,config().fieldValue(auth_protocol_shared_secret_config::max_challenge_size),config().fieldValue(auth_protocol_shared_secret_config::min_challenge_size));
    if (ec)
    {
        //! @todo critical: chain errors
        return ec;
    }

    // fill token
    auth_challenge_token::type token;
    token.field(auth_challenge_token::id).mutableValue()->generate();
    token.field(auth_challenge_token::token_created_at).mutableValue()->loadCurrentUtc();
    token.field(auth_challenge_token::expire).set(token.field(auth_challenge_token::token_created_at).value());
    token.field(auth_challenge_token::expire).mutableValue()->addSeconds(config().fieldValue(auth_protocol_shared_secret_config::token_ttl_secs));
    token.setFieldValue(auth_challenge_token::challenge,challenge->fieldValue(auth_hss_challenge::challenge));
    token.setFieldValue(auth_challenge_token::login,authRequest->fieldValue(auth_negotiate_request::login));
    if (!authRequest->fieldValue(auth_negotiate_request::topic).empty())
    {
        token.setFieldValue(auth_challenge_token::topic,authRequest->fieldValue(auth_negotiate_request::topic));
    }
    challenge->setFieldValue(auth_hss_challenge::expire,token.fieldValue(auth_challenge_token::expire));

    // serialize token
    du::WireBufSolid buf{factory};
    du::io::serialize(token,buf,ec);
    if (ec)
    {
        //! @todo critical: chain errors
        return ec;
    }

    // encrypt token
    auto& tokenField=response->field(auth_protocol_response::token);
    auto* tokenBuf=tokenField.buf(true);
    crypt::CryptContainer cipher{m_tokenEncryptionKey.get(),m_suite,factory};
    ec=cipher.pack(*buf.mainContainer(),*tokenBuf);
    if (ec)
    {
        //! @todo critical: chain errors
        return ec;
    }

    // done
    response->field(auth_protocol_response::content).set(std::move(challenge));
    return response;
}

//--------------------------------------------------------------------------

HATN_SERVERAPP_NAMESPACE_END
