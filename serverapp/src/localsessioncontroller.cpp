/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*
    
*/
/** @file serverapp/localsessioncontroller.cpp
  */

/****************************************************************************/

#include <hatn/serverapp/sessiondbmodels.h>
#include <hatn/serverapp/localsessioncontroller.h>

#include <hatn/dataunit/ipp/syntax.ipp>

HATN_SERVERAPP_NAMESPACE_BEGIN

//--------------------------------------------------------------------------

LocalSessionControllerBase::LocalSessionControllerBase(std::shared_ptr<SessionDbModels> sessionDbModels)
    : m_sessionDbModels(std::move(sessionDbModels))
{
}

//--------------------------------------------------------------------------

LocalSessionControllerBase::~LocalSessionControllerBase()
{}

//--------------------------------------------------------------------------

Error LocalSessionControllerBase::init(const crypt::CipherSuites* suites)
{
    SessionToken::TokenHandlers handlers;

    for (size_t i=0;i<config().field(session_config::tokens).count();i++)
    {
        const auto& tokenConfig=config().field(session_config::tokens).at(i);
        auto handler=std::make_shared<EncryptedToken>();

        const crypt::CipherSuite* suite=nullptr;
        if (tokenConfig.fieldValue(session_token::cipher_suite).empty())
        {
            suite=suites->defaultSuite();
        }
        else
        {
            suite=suites->suite(tokenConfig.fieldValue(session_token::cipher_suite));
        }
        if (suite==nullptr)
        {
            return common::genericError(fmt::format(_TR("unknown cipher suite \"{}\" for tag \"{}\""),
                                                    tokenConfig.fieldValue(session_token::cipher_suite),tokenConfig.fieldValue(session_token::tag)),
                                                    common::CommonError::CONFIGURATION_ERROR);
        }

        auto ec=handler->init(suite,tokenConfig.fieldValue(session_token::secret));
        HATN_CHECK_CHAIN_EC(ec,_TR(fmt::format("failed to initialize session tokens handler for tag \"{}\"",tokenConfig.fieldValue(session_token::tag))))

        handlers.emplace(tokenConfig.fieldValue(session_token::tag),std::move(handler));
    }

    auto ec=m_tokenHandler.init(std::string{config().fieldValue(session_config::current_tag)},std::move(handlers));
    HATN_CHECK_CHAIN_EC(ec,_TR("failed to initialize session tokens handler"))

    return OK;
}

//--------------------------------------------------------------------------

HATN_SERVERAPP_NAMESPACE_END
