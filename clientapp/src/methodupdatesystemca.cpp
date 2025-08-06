/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file clientapp/methodupdatesystemca.сpp
  *
  */

#include <hatn/dataunit/syntax.h>

#include <hatn/crypt/x509certificate.h>

#include <hatn/clientapp/methodupdatesystemca.h>
#include <hatn/clientapp/systemservice.h>

#include <hatn/dataunit/ipp/syntax.ipp>

HATN_CLIENTAPP_NAMESPACE_BEGIN

HDU_UNIT(system_ca_config,
    HDU_FIELD(format,TYPE_STRING,1,false,"PEM")
)

//---------------------------------------------------------------

void MethodUpdateSystemCa::exec(
        common::SharedPtr<HATN_APP_NAMESPACE::AppEnv> env,
        common::SharedPtr<Context> ctx,
        Request request,
        Callback callback
    )
{
    HATN_CTX_SCOPE("updatesystemca::exec")

    auto msg=request.message.as<system_ca_config::managed>();

    HATN_CTX_DEBUG_RECORDS(1,"system CA update",{"format",msg->fieldValue(system_ca_config::format)})

    auto format=crypt::ContainerFormat::PEM;
    if (msg->fieldValue(system_ca_config::format)=="DER")
    {
        format=crypt::ContainerFormat::DER;
    }
    else if (msg->fieldValue(system_ca_config::format)!="PEM")
    {
        callback(crypt::cryptError(crypt::CryptError::INVALID_CONTENT_FORMAT),Response{});
        return;
    }

    //! @todo load CA to global CA store

    const auto* suites=env->get<app::CipherSuites>().suites();
    if (suites==nullptr)
    {
        callback(clientAppError(ClientAppError::CIPHER_SUITES_UNDEFINED),Response{});
        return;
    }

    const auto* defaultSuite=suites->defaultSuite();
    if (defaultSuite==nullptr)
    {
        callback(clientAppError(ClientAppError::DEFAULT_CIPHER_SUITES_UNDEFINED),Response{});
        return;
    }

    size_t i=0;
    for (const auto& caData: request.buffers)
    {
        Error ec;
        auto ca=defaultSuite->createX509Certificate(ec);
        if (ec)
        {
            callback(common::chainError(std::move(ec),_TR(fmt::format("failed to create X.509 certificate for CA number {}",i))),Response{});
            return;
        }

        ec=ca->importFromBuf(caData->data(),caData->size(),format);
        if (ec)
        {
            callback(common::chainError(std::move(ec),_TR(fmt::format("failed to import X.509 certificate for CA number {}",i))),Response{});
            return;
        }

        i++;
    }

    HATN_CTX_DEBUG_RECORDS(1,"CA imported",{"number_of_ca",i})

    std::ignore=ctx;
    callback(Error{},Response{});
}

//---------------------------------------------------------------

std::string MethodUpdateSystemCa::messageType() const
{
    return system_ca_config::conf().name;
}

//---------------------------------------------------------------

MessageBuilderFn MethodUpdateSystemCa::messageBuilder() const
{
    return messageBuilderT<system_ca_config::managed>();
}

//---------------------------------------------------------------

HATN_CLIENTAPP_NAMESPACE_END

